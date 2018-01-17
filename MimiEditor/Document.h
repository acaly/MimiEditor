#pragma once
#include "EventHandler.h"
#include <cstdint>
#include <cstddef>

namespace Mimi
{
	class TextSegment;

	class LabelOwnerChangeEvent
	{
		friend class TextSegment;
	private:
		TextSegment* OldOwner;
		TextSegment* NewOwner;
		std::size_t BeginPosition;
		std::size_t EndPosition;
		std::ptrdiff_t IndexChange;
	};

	class Document
	{
		friend class TextSegment;

	public:
		std::size_t GetSnapshotCount();
		std::size_t GetSnapshotCapacity();

	public:
		Event<LabelOwnerChangeEvent> LabelOwnerChange;
	};
}
