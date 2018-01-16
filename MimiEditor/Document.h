#pragma once
#include "EventHandler.h"
#include <cstdint>

namespace Mimi
{
	class TextSegment;

	class LabelOwnerChangeEvent
	{
		friend class TextSegment;
	private:
		TextSegment* OldOwner;
		TextSegment* NewOwner;
		std::uint32_t BeginPosition;
		std::uint32_t EndPosition;
		std::int32_t IndexChange;
	};

	class Document
	{
		friend class TextSegment;

	public:
		std::uint32_t GetSnapshotCount();
		std::uint32_t GetSnapshotCapacity();

	public:
		Event<LabelOwnerChangeEvent> LabelOwnerChange;
	};
}
