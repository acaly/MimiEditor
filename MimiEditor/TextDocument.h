#pragma once
#include "EventHandler.h"
#include "TextSegmentList.h"
#include "ShortVector.h"
#include "CodePage.h"
#include "Clock.h"
#include <cstdint>
#include <cstddef>

namespace Mimi
{
	class TextSegment;
	class FileTypeDetector;
	class Snapshot;
	class DynamicBuffer;

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
		TextDocument(TextSegment* firstSegment) //Use factory
			: SegmentTree(this, firstSegment)
		{
		}
	public:
		TextDocument(const TextDocument&) = delete;
		TextDocument(TextDocument&&) = delete;
		TextDocument& operator= (const TextDocument&) = delete;
		virtual ~TextDocument();

	public:
		//Content access.
		TextSegmentTree SegmentTree;
		CodePage TextEncoding;

	private:
		std::size_t SnapshotCount;
		std::size_t SnapshotCapacity;
		std::size_t NextSnapshotIndex;
		ShortVector<bool> SnapshotInUse;
		Clock Timestamp;

	public:
		//Snapshot manipulation.
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

	public:
		//Label manipulation.
		Event<LabelOwnerChangedEvent, TextSegment*> LabelOwnerChanged;
		Event<LabelRemovedEvent, TextSegment*> LabelRemoved;

		//TODO global label functions (label type reg, etc.)
		//TODO label manipulation (add, remove, move, etc.)

	public:
		//Inter-segment modification.
		DocumentPositionS DeleteRange(std::uint32_t time, DocumentPositionS begin, DocumentPositionS end);
		void Insert(std::uint32_t time, DocumentPositionS pos, DynamicBuffer& content, bool hasNewline,
			DocumentPositionS* begin, DocumentPositionS* end);

	public:
		static TextDocument* CreateFromTextFile(FileTypeDetector* file);
	};
}
