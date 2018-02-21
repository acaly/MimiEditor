#include "ModificationTracer.h"
#include <cassert>

Mimi::ModificationTracer::ModificationTracer()
{
	this->SnapshotHead = nullptr;
}

Mimi::ModificationTracer::~ModificationTracer()
{
	Snapshot* s = this->SnapshotHead;
	while (s)
	{
		Snapshot* next = s->Next;
		delete s;
		s = next;
	}
}

void Mimi::ModificationTracer::Insert(std::size_t pos, std::size_t len)
{
	assert(pos < MaxLength && len <= MaxLength - pos);
	std::int32_t cpos = static_cast<std::int32_t>(pos);
	std::int16_t change = static_cast<std::int16_t>(len);

	ShortVector<Modification>& list = this->SnapshotHead->Modifications;

	//Find the point to insert
	Modification* head = list.GetPointer();
	Modification* m = head;
	std::int32_t delta = 0;
	while (m->Change) //Not the last
	{
		int i = m->Position + delta;

		if (i > cpos)
		{
			//Already after the position. We must insert now.
			list.Insert(m - head, { static_cast<std::uint16_t>(pos - delta), change });
			return;
		}
		if (i == cpos && m->Change > 0 || i + m->Change >= cpos)
		{
			//Merge the two insertions.
			m->Change += change;
			return;
		}
		if (i == cpos && m->Change < 0)
		{
			//Try to insert before delete if they are at the same position.
			list.Insert(m - head, { static_cast<std::uint16_t>(cpos - delta), change });
			return;
		}
		//Otherwise we can wait for next entry.
		delta += m->Change;
		m += 1;
	}
	//Reach the last entry.
	assert(m->Position + delta > cpos && "Modification tracer: insert after the end");
	list.Insert(m - head, { static_cast<std::uint16_t>(pos - delta), change });
}

void Mimi::ModificationTracer::Delete(std::size_t pos, std::size_t len)
{
	assert(pos < MaxLength && len <= MaxLength - pos);
	std::int32_t cpos = static_cast<std::int32_t>(pos);
	std::int16_t clen = static_cast<std::int16_t>(len);

	ShortVector<Modification>& list = this->SnapshotHead->Modifications;

	//Find the point to delete
	Modification* head = list.GetPointer();
	Modification* m = head;
	Modification* coverStart = nullptr;
	std::int32_t delta = 0;
	std::int32_t deltaStart = 0;
	std::int32_t extraChange = 0;

	while (m->Change) //Not the last
	{
		std::int32_t i = m->Position + delta;
		std::int32_t oldChange = m->Change; //It can change

		if (i > cpos + clen)
		{
			//Already after the range. Delete now.
			//Delete skipped items if any
			std::size_t insertPosition = m - head;
			if (coverStart)
			{
				list.RemoveRange(coverStart - head, m - coverStart);
				insertPosition = coverStart - head;
			}
			else
			{
				deltaStart = delta;
			}
			list.Insert(insertPosition, {
				static_cast<std::uint16_t>(cpos - deltaStart),
				static_cast<std::int16_t>(-clen + extraChange) });
			return;
		}
		if (i == cpos + clen && m->Change < 0)
		{
			//Right after the range, if it's also deletion, merge.
			//Note that if it's insertion, we must wait and delete after it.
			m->Change -= clen;
			m->Position = static_cast<std::uint16_t>(cpos - delta);

			//Delete skipped items if any
			if (coverStart)
			{
				list.RemoveRange(coverStart - head, m - coverStart);
			}
			return;
		}
		if (coverStart != nullptr || i >= cpos || i + m->Change > cpos)
		{
			if (coverStart == nullptr)
			{
				//The first covered item.
				coverStart = m;
				if (i >= cpos)
				{
					deltaStart = delta;
				}
				else if (i + m->Change < cpos + clen)
				{
					//Start within an insert and go beyond, fix these numbers.
					//Note that in the case it doesn't go beyond (the else in this if),
					//we have next 'Within the insertion' to deal with it.
					deltaStart = delta + (cpos - i);
					extraChange = -(cpos - i);
					//Update this ins and let's not remove it.
					m->Change = cpos - i;
					coverStart += 1;
				}
			}

			//Check if we can stop here.
			if (m->Change > 0 && i + m->Change >= cpos + clen)
			{
				if (i <= cpos)
				{
					assert(coverStart == m);
					//Within the insertion.
					m->Change -= clen;
					if (m->Change == 0)
					{
						list.RemoveAt(m - head);
					}
					return;
				}
				else
				{
					std::int32_t move = cpos + clen - i;
					std::int32_t oldPosition = m->Position;
					extraChange += move;
					//Trim the insertion (before modifying the list).
					m->Change -= move;
					//Move the insertion to before this deletion.
					//Note that we should consider extraChange and use clen - extraChange here.
					//  It's possible that i - cpos != clen - extraChange.
					//  This happens when extraChange changes because of other items.
					m->Position -= clen - extraChange;
					//After m moves backwards, it is possible that it can be merged with
					//another insertion. Merging with deletion is impossible because deletion
					//is considered 'covered' and planned to be removed.
					if (coverStart != head && coverStart[-1].Position == m->Position)
					{
						assert(coverStart[-1].Change > 0);
						coverStart[-1].Change += m->Change;
						m->Change = 0;
						//Now let the next part deal with it
					}

					//TODO try to merge the 4 cases
					if (m->Change == 0)
					{
						//There might be a deletion right after the insertion. Try to merge.
						//Change < 0 is necessary, as the next might be the end (Change == 0).
						if (m[1].Position == oldPosition && m[1].Change < 0)
						{
							m[1].Position += -clen + extraChange;
							m[1].Change += -clen + extraChange;
							//Remove those since coverStart. Also remove the insertion as it's zero length
							list.RemoveRange(coverStart - head, m - coverStart + 1);
						}
						else
						{
							//Remove those since coverStart. Also remove the insertion as it's zero length
							list.RemoveRange(coverStart - head, m - coverStart + 1);
							list.Insert(coverStart - head, {
								static_cast<std::uint16_t>(cpos - deltaStart),
								static_cast<std::int16_t>(-clen + extraChange) });
						}
					}
					else
					{
						//Delete (try to merge) 
						if (m[1].Position == oldPosition && m[1].Change < 0)
						{
							//Merge before modifying the list.
							m[1].Position += -clen + extraChange;
							m[1].Change += -clen + extraChange;
							//Remove those since coverStart (until the item before this one).
							list.RemoveRange(coverStart - head, m - coverStart);
						}
						else
						{
							//Remove those since coverStart (until the item before this one).
							list.RemoveRange(coverStart - head, m - coverStart);
							//Delete (+1 position to add it after the insertion originally at m)
							list.Insert(coverStart - head + 1, {
								static_cast<std::uint16_t>(cpos - deltaStart),
								static_cast<std::int16_t>(-clen + extraChange) });
						}
					}
					return;
				}
			}

			//Merge with existing changes (either insertion or deletion)
			extraChange += oldChange;
			assert(-clen + extraChange < 0);
		}

		delta += oldChange;
		m += 1;
	}

	//Reach the last entry
	assert(m->Position + delta >= cpos + clen && "Modification tracer: delete after the end");
	//Set coverStart if not yet
	if (coverStart == nullptr)
	{
		coverStart = m;
		deltaStart = delta;
	}
	else
	{
		//Remove those since coverStart (until the item before this one)
		list.RemoveRange(coverStart - head, m - coverStart);
	}
	//Delete
	list.Insert(coverStart - head, {
		static_cast<std::uint16_t>(cpos - deltaStart),
		static_cast<std::int16_t>(-clen + extraChange) });
}

void Mimi::ModificationTracer::ExchangeFront(std::size_t oldNum)
{
	if (oldNum == 0)
	{
		return;
	}
	Snapshot* s = this->SnapshotHead;
	for (std::size_t i = 1; i < oldNum; ++i)
	{
		s = s->Next;
	}
	Snapshot* exchange = s->Next;
	//The caller ensures there are enough nodes: exchange != nullptr.
	s->Next = exchange->Next;
	exchange->Next = this->SnapshotHead;
	this->SnapshotHead = exchange;
}

void Mimi::ModificationTracer::NewSnapshot(std::size_t newSnapshotNum, std::size_t length)
{
	assert(length < MaxLength);
	this->ExchangeFront(newSnapshotNum - 1);
	this->SnapshotHead->Modifications.Clear();
	this->SnapshotHead->Modifications.Shink(2);
	this->SnapshotHead->Modifications.Append({ static_cast<std::uint16_t>(length), 0 });
}

void Mimi::ModificationTracer::DisposeSnapshot(std::size_t oldNum, std::size_t num)
{
	//Do nothing
}

void Mimi::ModificationTracer::Resize(std::size_t size)
{
	Snapshot* s = this->SnapshotHead;
	Snapshot* last = nullptr;
	for (std::size_t i = 0; i < size; ++i)
	{
		if (s == nullptr)
		{
			s = new Snapshot();
			s->Next = nullptr;
			if (i == 0)
			{
				this->SnapshotHead = s;
			}
			else
			{
				last->Next = s;
			}
		}
		last = s;
		s = s->Next;
	}
	while (s)
	{
		Snapshot* next = s->Next;
		delete s;
		s = next;
	}
}

std::size_t Mimi::ModificationTracer::GetSnapshotLength(std::size_t snapshot)
{
	Snapshot* s = SnapshotHead;
	for (std::size_t i = 0; i < snapshot; ++i)
	{
		s = s->Next;
	}
	return s->Modifications[s->Modifications.GetCount() - 1].Position;
}

std::size_t Mimi::ModificationTracer::ConvertFromSnapshotSingle(Snapshot* snapshot, std::size_t pos, int dir)
{
	assert(pos < MaxLength);
	std::int32_t cpos = static_cast<std::int32_t>(pos);
	Modification* head = snapshot->Modifications.GetPointer();
	Modification* m = head;
	std::int32_t delta = 0;
	while (m->Change)
	{
		if (m->Position > cpos || m->Position - m->Change > cpos)
		{
			//For insertion, m->Position must be after pos
			//For deletion, m->Position - m->Change is the end, which must be after pos.
			if (m->Change > 0)
			{
				//Insertion after pos.
				return static_cast<std::size_t>(cpos + delta);
			}
			else
			{
				if (m->Position > cpos)
				{
					//Deletion after pos
					return static_cast<std::size_t>(cpos + delta);
				}
				else
				{
					//The char is deleted.
					if (dir == 0) return PositionDeleted;
					if (dir < 0) return static_cast<std::size_t>(m->Position + delta);
					//dir > 0
					if (m == head ||
						m[-1].Change < 0 ||
						m[-1].Position != m->Position)
					{
						//Note that this may produce (size_t)-1, which gives 0xFFFFFFFF (not found).
						return static_cast<std::size_t>(m->Position + delta - 1);
					}
					else
					{
						//Let it go before the adjacent insertion.
						return static_cast<std::size_t>(m->Position + delta - m[-1].Change - 1);
					}
				}
			}
		}
		delta += m->Change;
		m += 1;
	}
	assert(m->Position > cpos);
	return static_cast<std::size_t>(cpos + delta);
}

std::size_t Mimi::ModificationTracer::FirstModifiedFromSnapshot(std::size_t snapshot)
{
	Snapshot* s = this->SnapshotHead;
	std::size_t pos = PositionDeleted;
	s = s->Next;
	for (std::size_t i = 1; i < snapshot; ++i)
	{
		if (s->Modifications[0].Change != 0)
		{
			std::size_t newPos = s->Modifications[0].Position;
			if (pos == PositionDeleted || newPos < pos) pos = newPos;
		}
		s = s->Next;
	}
	return pos;
}

void Mimi::ModificationTracer::MergeWith(ModificationTracer& other, std::size_t snapshots)
{
	Snapshot* sa = this->SnapshotHead;
	Snapshot* sb = other.SnapshotHead;
	for (std::size_t i = 0; i < snapshots; ++i)
	{
		Modification* maLast = &sa->Modifications.GetPointer()[sa->Modifications.GetCount() - 1];

		//Find the offset for b.
		Modification* ma = maLast;
		std::uint16_t offset = ma->Position;

		//Variables needed to add merged entries.
		std::int16_t mergeDelete = 0;
		std::int16_t mergeInsert = 0;
		std::uint16_t mergePosition = offset;

		//Check last item.
		if (ma - 1 >= sa->Modifications.GetPointer())
		{
			std::int32_t length = offset;
			Modification* mfind = ma - 1;
			//Check deletion first.
			if (mfind->Change < 0)
			{
				std::int32_t endPos = mfind->Position;
				endPos -= mfind->Change;
				if (endPos == length)
				{
					mergeDelete = mfind->Change; //Remember change.
					length = mfind->Position; //Check insertion at new position.
					if (mfind - 1 >= sa->Modifications.GetPointer())
					{
						//Possible insertion is before this entry.
						//Note that if it's the first entry, later the check for
						//insertion must fail (Change > 0).
						mfind -= 1;
					}
					ma -= 1; //Remove this entry.
				}
				else
				{
					//We don't need to check insertion if ever goes here.
					//But the presence of deletion ensures previous insertion (if
					//any) does not meet the condition and will not be processed.
				}
			}
			//Then check insertion.
			if (mfind->Change > 0)
			{
				std::int32_t endPos = mfind->Position;
				if (endPos == length)
				{
					mergeInsert = mfind->Change;
					ma -= 1;
				}
			}
			mergePosition = length;
		}

		//Remove the checked entries (including the count entry).
		sa->Modifications.RemoveRange(ma - sa->Modifications.GetPointer(), maLast - ma + 1);

		Modification* mb = sb->Modifications.GetPointer();
		int ifind = 0;
		//Check the start of b.
		if (sb->Modifications.GetCount() > 1)
		{
			//Check insertion first.
			if (mb->Change > 0 && mb->Position == 0)
			{
				mergeInsert += mb->Change;
				//We don't need to worry about bound check here, because
				//the last entry must have Change == 0.
				mb += 1; //Check at next entry, and also don't copy this.
			}
			if (mb->Change < 0 && mb->Position == 0)
			{
				mergeDelete += mb->Change;
				mb += 1; //Don't copy this entry.
			}
		}

		//Add merged entries if needed.
		if (mergeInsert != 0)
		{
			sa->Modifications.Append({ mergePosition, mergeInsert });
		}
		if (mergeDelete != 0)
		{
			sa->Modifications.Append({ mergePosition, mergeDelete });
		}

		//Update the position for b.
		Modification* update = mb;
		while (update->Change)
		{
			update->Position += offset;
			update += 1;
		}
		update->Position += offset;
		//update is the last entry to copy.

		//Copy data from b.
		sa->Modifications.ApendRange(mb, update - mb + 1);

		//Merge next snapshot.
		sa = sa->Next;
		sb = sb->Next;
	}
}

void Mimi::ModificationTracer::SplitInto(ModificationTracer& other, std::size_t snapshots, std::size_t pos)
{
	assert(pos < MaxLength);
	Snapshot* sa = this->SnapshotHead;
	Snapshot* sb = other.SnapshotHead;
	std::int32_t p = static_cast<std::int32_t>(pos);

	for (std::size_t i = 0; i < snapshots; ++i)
	{
		sb->Modifications.Clear();

		Modification* heada = sa->Modifications.GetPointer();
		Modification* ma = heada;
		std::int32_t delta = 0;
		while (true)
		{
			std::int32_t i = ma->Position + delta;
			if (ma->Change == 0)
			{
				//Split after any change.
				std::int32_t keep = p - delta;
				std::int32_t move = ma->Position - keep;
				assert(move > 0 && "ModificationTracer: split after end.");
				ma->Position -= move;
				sb->Modifications.Append({ static_cast<std::uint16_t>(move), 0 });
				p = keep;
				break;
			}
			else if (ma->Change > 0)
			{
				if (i >= p)
				{
					//Split before an insertion
					std::int32_t keep = p - delta;

					//Modify the second part and find the count
					Modification* e = ma;
					while (e->Change)
					{
						e->Position -= keep;
						e += 1;
					}
					e->Position -= keep; //The last item.

					sb->Modifications.ApendRange(ma, e + 1 - ma); //Move
					sa->Modifications.RemoveRange(ma - heada, e + 1 - ma);
					sa->Modifications.Append({ static_cast<std::uint16_t>(keep), 0 }); //End sa
					p = keep;
					break;
				}
				else if (i + ma->Change > p)
				{
					//Split inside an insertion
					std::int32_t move = i + ma->Change - p;
					std::int32_t keep = ma->Position;

					//Modify the second part and find the count
					Modification* e = ma + 1; //skip ma
					while (e->Change)
					{
						e->Position -= keep;
						e += 1;
					}
					e->Position -= keep;

					sb->Modifications.Append({ 0, static_cast<std::int16_t>(move) }); //Split the insertion
					sb->Modifications.ApendRange(ma + 1, e - ma); //Move
					ma->Change -= move; //Split the insertion
					sa->Modifications.RemoveRange(ma + 1 - heada, e - ma);
					sa->Modifications.Append({ static_cast<std::uint16_t>(keep), 0 }); //End sa
					p = keep;
					break;
				}
			}
			else //ma->Change < 0
			{
				if (i > p)
				{
					//Split before an deletion (same as before insertion)
					std::int32_t keep = p - delta;

					//Modify the second part and find the count
					Modification* e = ma;
					while (e->Change)
					{
						e->Position -= keep;
						e += 1;
					}
					e->Position -= keep; //The last item.

					sb->Modifications.ApendRange(ma, e + 1 - ma); //Move
					sa->Modifications.RemoveRange(ma - heada, e + 1 - ma);
					sa->Modifications.Append({ static_cast<std::uint16_t>(keep), 0 }); //End sa
					p = keep;
					break;
				}
			}

			delta += ma->Change;
			ma += 1;
		}

		//Try to make it smaller.
		sb->Modifications.Shink();

		//Merge next snapshot.
		sa = sa->Next;
		sb = sb->Next;
	}
}

bool Mimi::ModificationTracer::CheckSequence(std::size_t numSnapshot, std::size_t rangeCheck)
{
	Snapshot* s = this->SnapshotHead;
	for (std::size_t i = 0; i < numSnapshot; ++i)
	{
		Modification* m = s->Modifications.GetPointer();
		if (m->Position > rangeCheck) return false;
		if (m->Change)
		{
			Modification* last = m;
			m += 1;
			while (m->Change)
			{
				if (m->Position > rangeCheck) return false;

				if (m->Change > 0)
				{
					if (last->Change > 0)
					{
						if (last->Position >= m->Position) return false;
					}
					else
					{
						if (last->Position - last->Change >= m->Position) return false;
					}
				}
				else
				{
					if (last->Change > 0)
					{
						if (last->Position > m->Position) return false;
					}
					else
					{
						if (last->Position - last->Change >= m->Position) return false;
					}
				}
				last = m;
				m += 1;
			}
			if (m->Position > rangeCheck) return false;
		}
		s = s->Next;
	}
	return true;
}
