#pragma once
#include "TextDocumentPosition.h"
#include "TextDocument.h"
#include <cstdint>

namespace Mimi
{
	class TextDocumentLabelIterator final
	{
	public:
		TextDocumentLabelIterator(TextDocument* document, std::uint16_t handler, int direction);
		TextDocumentLabelIterator(const TextDocumentLabelIterator&) = delete;
		TextDocumentLabelIterator(TextDocumentLabelIterator&&) = delete;
		TextDocumentLabelIterator& operator= (const TextDocumentLabelIterator&) = delete;
		~TextDocumentLabelIterator();

	public:
		DocumentPositionS GetCurrentPosition();
		bool GoNext();

		void SetPosition(DocumentPositionS pos);
	};
}
