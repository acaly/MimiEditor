#pragma once
#include "EventHandler.h"
#include "TextSegmentList.h"
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

	public:
		std::size_t GetSnapshotCount();
		std::size_t GetSnapshotCapacity();
		Snapshot* CreateSnapshot();
		void DisposeSnapshot(Snapshot* s);
		//TODO convert snapshot position
		//TODO global label functions (label type reg, etc.)

	public:
		Event<LabelOwnerChangedEvent, TextSegment*> LabelOwnerChanged;
		Event<LabelRemovedEvent, TextSegment*> LabelRemoved;

	public:
		static TextDocument* CreateFromTextFile(FileTypeDetector* file);
	};
}
