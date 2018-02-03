#pragma once
#include "TextDocumentPosition.h"
#include <cstdint>

namespace Mimi
{
	struct TextDocumentLabelAccess
	{
		TextDocumentLabelAccess(DocumentLabelIndex label);

	public:
		std::uint8_t GetLabelType();
		std::uint16_t GetHandlerId();

		bool HasLongData();
		std::uint8_t& ShortData();
		std::uint32_t& LongData();

		DocumentPositionS GetBeginPosition();
		DocumentPositionS GetEndPosition();
	};
}
