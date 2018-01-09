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
	list.EnsureExtra(1); //We need to add 1 more entry. Resize now to avoid relocation.

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
	list.EnsureExtra(1); //We need to add 1 more entry. Resize now to avoid relocation.

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
			list.Insert(m - head, { static_cast<std::uint16_t>(cpos - delta), -clen });
			return;
		}
		if (i == cpos + clen)
		{
			//Right after the range, if it's also deletion, merge.
			if (m->Change < 0)
			{
				m->Change -= clen;
				m->Position = static_cast<std::uint16_t>(cpos - delta);
			}
			else
			{
				//Can't merge. Just delete.
				list.Insert(m - head, { static_cast<std::uint16_t>(cpos - delta), -clen });
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
					//Note that in the case it doesn't go beyond,
					//we have next 'del is within the ins' to deal with it.
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
				if (i < cpos)
				{
					//Within the insertion.
					m->Change -= clen;
					return;
				}
				else
				{
					int move = cpos + clen - i;
					extraChange += move;
					//Trim the insertion (before modifying the list).
					m->Change -= move;
					//Move the insertion to before this deletion.
					m->Position -= (i - cpos);
					//Remove those since coverStart (until the item before this one).
					list.RemoveRange(coverStart - head, m - coverStart);
					//Delete (+1 position to add it after the insertion originally at m)
					list.Insert(coverStart - head + 1, {
						static_cast<std::uint16_t>(cpos - deltaStart),
						static_cast<std::int16_t>(-clen + extraChange) });
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
	//Remove those since coverStart (until the item before this one)
	list.RemoveRange(coverStart - head, m - coverStart);
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
