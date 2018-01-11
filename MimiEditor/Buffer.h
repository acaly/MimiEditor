#pragma once
#include <cstdint>
#include "ShortVector.h"

namespace Mimi
{
	struct StaticBuffer
	{
	private:
		struct StaticBufferData
		{
			std::uint16_t RefCount;
			std::uint16_t Size;
			std::uint8_t RawData[1];
		};

		StaticBufferData* Data;

		//TODO Copy and Dispose
	};

	typedef ShortVector<std::uint8_t> DynamicBuffer;
}
