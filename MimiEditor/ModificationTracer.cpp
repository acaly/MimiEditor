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

void Mimi::ModificationTracer::Insert(std::uint32_t pos, std::uint32_t len)
{
	int cpos = static_cast<int>(pos);
	std::int16_t change = static_cast<std::int16_t>(len);

	ShortVector<Modification>& list = this->SnapshotHead->Modifications;

	//Find the point to insert
	Modification* head = list.GetPointer();
	Modification* m = head;
	int delta = 0;
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

void Mimi::ModificationTracer::Delete(std::uint32_t pos, std::uint32_t len)
{
	int cpos = static_cast<int>(pos);
	std::int16_t clen = static_cast<std::int16_t>(len);

	ShortVector<Modification>& list = this->SnapshotHead->Modifications;

	//Find the point to delete
	Modification* head = list.GetPointer();
	Modification* m = head;
	int delta = 0;
	int deltaStart = 0;
	Modification* coverStart = nullptr;
	int extraChange = 0;

	while (m->Change) //Not the last
	{
		int i = m->Position + delta;
		int oldChange = m->Change; //It can change

		if (i > cpos + clen)
		{
			//Already after the range. Delete now.
			//Delete skipped items if any
			int insertPosition = m - head;
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
					int move = cpos + clen - i;
					int oldPosition = m->Position;
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
	assert(m->Position + delta > cpos + clen && "Modification tracer: delete after the end");
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

void Mimi::ModificationTracer::ExchangeFront(std::uint32_t oldNum)
{
	if (oldNum == 0)
	{
		return;
	}
	Snapshot* s = this->SnapshotHead;
	for (std::uint32_t i = 1; i < oldNum; ++i)
	{
		s = s->Next;
	}
	Snapshot* exchange = s->Next;
	//The caller ensures there are enough nodes: exchange != nullptr.
	s->Next = exchange->Next;
	exchange->Next = this->SnapshotHead;
	this->SnapshotHead = exchange;
}

void Mimi::ModificationTracer::NewSnapshot(std::uint32_t newSnapshotNum, std::uint32_t length)
{
	this->ExchangeFront(newSnapshotNum - 1);
	this->SnapshotHead->Modifications.Clear();
	//TODO shink the vector length here
	this->SnapshotHead->Modifications.Append({ static_cast<std::uint16_t>(length), 0 });
}

void Mimi::ModificationTracer::DisposeSnapshot(std::uint32_t oldNum, std::uint32_t num)
{
	//Do nothing
}

void Mimi::ModificationTracer::Resize(std::uint32_t size)
{
	Snapshot* s = this->SnapshotHead;
	for (std::uint32_t i = 0; i < size; ++i)
	{
		if (s == nullptr)
		{
			s = new Snapshot();
			s->Next = nullptr;
			if (i == 0)
			{
				this->SnapshotHead = s;
			}
		}
		s = s->Next;
	}
	while (s)
	{
		Snapshot* next = s->Next;
		delete s;
		s = next;
	}
}

std::uint32_t Mimi::ModificationTracer::ConvertFromSnapshotSingle(Snapshot* snapshot, std::uint32_t pos, int dir)
{
	int cpos = static_cast<int>(pos);
	Modification* head = snapshot->Modifications.GetPointer();
	Modification* m = head;
	int delta = 0;
	while (m->Change)
	{
		if (m->Position > cpos || m->Position - m->Change > cpos)
		{
			//For insertion, m->Position must be after pos
			//For deletion, m->Position - m->Change is the end, which must be after pos.
			if (m->Change > 0)
			{
				//Insertion after pos.
				return static_cast<std::uint32_t>(cpos + delta);
			}
			else
			{
				if (m->Position > cpos)
				{
					//Deletion after pos
					return static_cast<std::uint32_t>(cpos + delta);
				}
				else
				{
					//The char is deleted.
					if (dir == 0) return PositionDeleted;
					if (dir < 0) return static_cast<std::uint32_t>(m->Position + delta);
					//dir > 0
					if (m == head ||
						m[-1].Change < 0 ||
						m[-1].Position != m->Position)
					{
						//Note that this may produce (int)-1, which gives 0xFFFFFFFF (not found).
						return static_cast<std::uint32_t>(m->Position + delta - 1);
					}
					else
					{
						//Let it go before the adjacent insertion.
						return static_cast<std::uint32_t>(m->Position + delta - m[-1].Change - 1);
					}
				}
			}
		}
		delta += m->Change;
		m += 1;
	}
	assert(m->Position > cpos);
	return static_cast<std::uint32_t>(cpos + delta);
}

std::uint32_t Mimi::ModificationTracer::FirstModifiedFromSnapshot(std::uint32_t snapshot)
{
	Snapshot* s = this->SnapshotHead;
	int pos = s->Modifications[0].Position;
	s = s->Next;
	for (std::uint32_t i = 1; i < snapshot; ++i)
	{
		int newPos = s->Modifications[0].Position;
		if (newPos < pos) pos = newPos;
		s = s->Next;
	}
	return pos;
}

bool Mimi::ModificationTracer::CheckSequence(std::uint32_t numSnapshot, std::uint32_t rangeCheck)
{
	Snapshot* s = this->SnapshotHead;
	for (std::uint32_t i = 0; i < numSnapshot; ++i)
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
