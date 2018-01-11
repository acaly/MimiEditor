#pragma once

#include "CommonInternal.h"
#include "ShortVector.h"
#include <cstdint>

namespace Mimi
{
	//Trace the modification of one line between snapshots
	//Current implementation assumes a maximum length of ? (16 bit)
	//and maximum snapshot number of 255 (8 bit).
	//Optimization considerations:
	//  Usually there is only 0 or 1 snapshot. (More snapshots can be slower.)
	//  Number of modification is small (<5)
	//In order to reduce memory usage, this class does not track the number
	//of snapshot. The number and size must be correctly managed by caller.
	//In order to reduce the allocation and memory movement, while at the same
	//time allowing temporarily increasing to 2 (or even more) snapshots, the
	//linked list is used here, although changing the size requires exchange of
	//nodes.
	class ModificationTracer final
	{
	public:
		ModificationTracer();
		ModificationTracer(const ModificationTracer&) = delete;
		ModificationTracer(ModificationTracer&&) = delete;
		~ModificationTracer();

	private:
		struct Modification
		{
			std::uint16_t Position;
			std::int16_t Change;
		};

		struct Snapshot
		{
			Snapshot* Next = nullptr;

			//The last item is always a change=0, indicating
			//the range of this Tracer (in terms of old length).
			ShortVector<Modification> Modifications;
		};

		Snapshot* SnapshotHead;

	public:
		static const std::uint32_t PositionDeleted = 0xFFFFFFFF;

	public:
		void Insert(std::uint32_t pos, std::uint32_t len);
		void Delete(std::uint32_t pos, std::uint32_t len);

	private:
		void ExchangeFront(std::uint32_t oldNum);

	public:
		void NewSnapshot(std::uint32_t newSnapshotNum, std::uint32_t length);
		void DisposeSnapshot(std::uint32_t oldNum, std::uint32_t num);
		void Resize(std::uint32_t newCapacity);

	private:
		std::uint32_t ConvertFromSnapshotSingle(Snapshot* snapshot, std::uint32_t pos, int dir);
		std::uint32_t ConvertToSnapshotSingle(Snapshot* snapshot, std::uint32_t pos, int dir);

	private:
		std::uint32_t ConvertFromSnapshotInternal(Snapshot* snapshot,
			std::uint32_t nsnapshot, std::uint32_t pos, int dir)
		{
			if (nsnapshot == 0) return pos;
			if (nsnapshot == 1) return ConvertFromSnapshotSingle(snapshot, pos, dir);
			auto remaining = ConvertFromSnapshotInternal(snapshot->Next, nsnapshot - 1, pos, dir);
			if (remaining == PositionDeleted) return PositionDeleted;
			return ConvertFromSnapshotSingle(snapshot, remaining, dir);
		}

	public:
		//pos: attached character
		//dir: attached side.
		//  -1 to attach to left (move right when deleted)
		//  +1 to attach to right (move left when deleted)
		//  0 to attach to the char (return PositionDeleted when deleted)
		std::uint32_t ConvertFromSnapshot(std::uint32_t snapshot, std::uint32_t pos, int dir)
		{
			if (snapshot == 0)
			{
				return ConvertFromSnapshotSingle(SnapshotHead, pos, dir);
			}
			return ConvertFromSnapshotInternal(SnapshotHead, snapshot + 1, pos, dir);
		}

		std::uint32_t ConvertToSnapshot(std::uint32_t snapshot, std::uint32_t pos, int dir)
		{
			auto newPos = ConvertToSnapshotSingle(SnapshotHead, pos, dir);
			if (snapshot == 0 || newPos == PositionDeleted)
			{
				return newPos;
			}
			Snapshot* s = SnapshotHead->Next;
			for (std::uint32_t i = 0; i < snapshot; ++i)
			{
				newPos = ConvertToSnapshotSingle(s, newPos, dir);
				if (newPos == PositionDeleted) return PositionDeleted;
				s = s->Next;
			}
			return newPos;
		}

		std::uint32_t FirstModifiedFromSnapshot(std::uint32_t snapshot);

	public:
		//Merge this tracer with 'other'.
		//All snapshots are merged (number given in 'snapshot'). After this, 'other' can be
		//deleted without losing any information.
		void MergeWith(ModificationTracer& other, std::uint32_t snapshots);

		//Split this tracer at position given in 'pos'. The data after this position is
		//transfered to 'other' (must be initialized with proper capacity before calling this
		//function). All snapshots are split at proper position.
		//Note that the initialization of 'other' is capacity (by Resize), not number of
		//snapshots. Snapshot items will be modified from the beginning.
		void SplitInto(ModificationTracer& other, std::uint32_t snapshots, std::uint32_t pos);

	public:
		//Used in test and debug cases only.
		bool CheckSequence(std::uint32_t numSnapshot, std::uint32_t rangeCheck);
	};
}
