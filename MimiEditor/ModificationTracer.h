#pragma once
#include "ShortVector.h"
#include <cstdint>
#include <cstddef>
#include <limits>

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
		static const std::size_t MaxLength = 0x7FFF;
		static const std::size_t MaxSnapshot = 0xFF;

	public:
		ModificationTracer();
		ModificationTracer(const ModificationTracer&) = delete;
		ModificationTracer(ModificationTracer&&) = delete;
		ModificationTracer& operator= (const ModificationTracer&) = delete;
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
		static const std::size_t PositionDeleted = std::numeric_limits<std::size_t>::max();

	public:
		void Insert(std::size_t pos, std::size_t len);
		void Delete(std::size_t pos, std::size_t len);

	private:
		void ExchangeFront(std::size_t oldNum);

	public:
		void NewSnapshot(std::size_t newSnapshotNum, std::size_t length);
		void DisposeSnapshot(std::size_t oldNum, std::size_t num);
		void Resize(std::size_t newCapacity);

	private:
		std::size_t ConvertFromSnapshotSingle(Snapshot* snapshot, std::size_t pos, int dir);
		std::size_t ConvertToSnapshotSingle(Snapshot* snapshot, std::size_t pos, int dir);

	private:
		std::size_t ConvertFromSnapshotInternal(Snapshot* snapshot,
			std::size_t nsnapshot, std::size_t pos, int dir)
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
		std::size_t ConvertFromSnapshot(std::size_t snapshot, std::size_t pos, int dir)
		{
			if (snapshot == 0)
			{
				return ConvertFromSnapshotSingle(SnapshotHead, pos, dir);
			}
			return ConvertFromSnapshotInternal(SnapshotHead, snapshot + 1, pos, dir);
		}

		std::size_t ConvertToSnapshot(std::size_t snapshot, std::size_t pos, int dir)
		{
			auto newPos = ConvertToSnapshotSingle(SnapshotHead, pos, dir);
			if (snapshot == 0 || newPos == PositionDeleted)
			{
				return newPos;
			}
			Snapshot* s = SnapshotHead->Next;
			for (std::size_t i = 0; i < snapshot; ++i)
			{
				newPos = ConvertToSnapshotSingle(s, newPos, dir);
				if (newPos == PositionDeleted) return PositionDeleted;
				s = s->Next;
			}
			return newPos;
		}

		std::size_t FirstModifiedFromSnapshot(std::size_t snapshot);

	public:
		//Merge this tracer with 'other'.
		//All snapshots are merged (number given in 'snapshot'). After this, 'other' can be
		//deleted without losing any information.
		void MergeWith(ModificationTracer& other, std::size_t snapshots);

		//Split this tracer at position given in 'pos'. The data after this position is
		//transfered to 'other' (must be initialized with proper capacity before calling this
		//function). All snapshots are split at proper position.
		//Note that the initialization of 'other' is capacity (by Resize), not number of
		//snapshots. Snapshot items will be modified from the beginning.
		void SplitInto(ModificationTracer& other, std::size_t snapshots, std::size_t pos);

	public:
		//Used in test and debug cases only.
		bool CheckSequence(std::size_t numSnapshot, std::size_t rangeCheck);
	};
}
