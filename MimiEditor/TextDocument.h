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
		std::size_t SingleId;

	public:
		bool Update(DocumentLabelIndex* label);
	};

	class LabelRemovedEvent
	{
		friend class TextSegment;

	private:
		TextSegment* Owner;
		std::size_t Index;
		std::size_t GlobalPosition;

	public:
		bool IsRemoved(DocumentLabelIndex label)
		{
			return label.Segment == Owner && label.Index == Index;
		}

		//This is the global position of the segment, but the API user can
		//easily calculate the position of the label based on the label data,
		//which has not been erased when this event is fired.
		std::size_t GetSegmentGlobalPosition()
		{
			return GlobalPosition;
		}
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
		Clock Timestamp;

	private:
		std::size_t SnapshotCount = 0;
		std::size_t SnapshotCapacity = 2;
		std::size_t NextSnapshotIndex = 0;
		ShortVector<bool> SnapshotInUse;
		std::uint16_t NextLabelHandlerIndex = 0;

	public:
		std::uint32_t GetTime()
		{
			return Timestamp.GetElapsedMinute<std::uint32_t>();
		}

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

		std::uint16_t GetFreeLabelHandler()
		{
			return NextLabelHandlerIndex++;
		}

		DocumentLabelIndex AddPointLabel(std::uint16_t handler, DocumentPositionS pos,
			int direction, bool longData, bool referred);
		DocumentLabelIndex AddLineLabel(std::uint16_t handler, DocumentPositionS pos,
			bool longData, bool referred);
		DocumentLabelIndex AddRangeLabel(std::uint16_t handler, DocumentPositionS begin,
			DocumentPositionS end, bool longData, bool referred);
		void DeleteLabel(DocumentLabelIndex label);
		//TODO move?

	public:
		//Inter-segment modification.
		DocumentPositionS DeleteRange(std::uint32_t time, DocumentPositionS begin, DocumentPositionS end);
		void Insert(std::uint32_t time, DocumentPositionS pos, DynamicBuffer& content, bool hasNewline,
			DocumentPositionS* begin, DocumentPositionS* end);

	public:
		static TextDocument* CreateEmpty(CodePage cp);
		static TextDocument* CreateFromTextFile(FileTypeDetector* file);
	};
}
