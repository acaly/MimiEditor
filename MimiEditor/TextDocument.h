#pragma once
#include "EventHandler.h"
#include "TextSegmentList.h"
#include "ShortVector.h"
#include <cstdint>
#include <cstddef>

namespace Mimi
{
	class TextSegment;
	class FileTypeDetector;
	class Snapshot;

	class LabelOwnerChangedEvent
	{
		friend class TextSegment;
	private:
		TextSegment* OldOwner;
		TextSegment* NewOwner;
		std::size_t BeginPosition;
		std::size_t EndPosition;
		std::ptrdiff_t IndexChange;
	};

	class LabelRemovedEvent
	{
		friend class TextSegment;
	private:
		TextSegment* Owner;
		std::size_t Index;
	};

	class TextDocument final
	{
		friend class TextSegment;

	private:
		TextDocument(); //Use factory
	public:
		TextDocument(const TextDocument&) = delete;
		TextDocument(TextDocument&&) = delete;
		TextDocument& operator= (const TextDocument&) = delete;
		virtual ~TextDocument();

	public:
		TextSegmentTree SegmentTree;
	private:
		std::size_t SnapshotCount;
		std::size_t SnapshotCapacity;
		std::size_t NextSnapshotIndex;
		ShortVector<bool> SnapshotInUse;

	public:
		std::size_t GetSnapshotCount()
		{
			return SnapshotCount;
		}

		std::size_t GetSnapshotCapacity()
		{
			return SnapshotCapacity;
		}

		Snapshot* CreateSnapshot();
		void DisposeSnapshot(Snapshot* s);

		std::size_t ConvertSnapshotToIndex(std::size_t hindex)
		{
			return NextSnapshotIndex - hindex - 1;
		}

		//TODO global label functions (label type reg, etc.)

	public:
		Event<LabelOwnerChangedEvent, TextSegment*> LabelOwnerChanged;
		Event<LabelRemovedEvent, TextSegment*> LabelRemoved;

	public:
		static TextDocument* CreateFromTextFile(FileTypeDetector* file);
	};
}
