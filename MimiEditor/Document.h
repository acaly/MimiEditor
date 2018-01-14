#pragma once
#include <cstdint>

namespace Mimi
{
	class Document
	{
	public:
		std::uint32_t GetSnapshotCount();
		std::uint32_t GetSnapshotCapacity();
	};
}
