#pragma once
#include "EventHandler.h"
#include <cstdint>
#include <cstddef>

namespace Mimi
{
	class TextSegment;

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

	class Document
	{
		friend class TextSegment;

	public:
		std::size_t GetSnapshotCount();
		std::size_t GetSnapshotCapacity();

	public:
		//TODO add event filter (i.e. for Label events, filter is TextSegment*)
		Event<LabelOwnerChangedEvent> LabelOwnerChanged;
		Event<LabelRemovedEvent> LabelRemoved;
	};
}
