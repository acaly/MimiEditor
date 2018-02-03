#pragma once
#include "TextDocumentPosition.h"

namespace Mimi
{
	class TextDocumentLabelTracer final
	{
	public:
		TextDocumentLabelTracer();
		TextDocumentLabelTracer(const TextDocumentLabelTracer&) = delete;
		TextDocumentLabelTracer(TextDocumentLabelTracer&&) = delete;
		TextDocumentLabelTracer& operator= (const TextDocumentLabelTracer&) = delete;
		~TextDocumentLabelTracer();

	public:
		void Set(DocumentLabelIndex label);
		DocumentLabelIndex Get();
	};
}
