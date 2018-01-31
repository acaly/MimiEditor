#pragma once
#include <cstddef>

namespace Mimi
{
	class TextSegment;

	struct DocumentPositionS
	{
		TextSegment* Segment;
		std::size_t Position;
	};

	struct DocumentPositionL
	{
		std::size_t Line;
		std::size_t Position;
	};

	struct DocumentPositionD
	{
		std::size_t Position;
	};
}
