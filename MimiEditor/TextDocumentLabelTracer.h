#pragma once
#include "TextDocumentPosition.h"
#include "TextDocument.h"
#include "TextSegment.h"

namespace Mimi
{
	class TextDocumentLabelTracer final
	{
	public:
		TextDocumentLabelTracer()
		{
			Document = nullptr;
		}

		TextDocumentLabelTracer(const TextDocumentLabelTracer&) = delete;
		TextDocumentLabelTracer(TextDocumentLabelTracer&&) = delete;
		TextDocumentLabelTracer& operator= (const TextDocumentLabelTracer&) = delete;

		~TextDocumentLabelTracer()
		{
			Clear();
		}

	private:
		TextDocument* Document;
		HandlerId HandlerOwner, HandlerRemove;
		DocumentLabelIndex LabelRef;
		
	private:
		void HandleOwnerChanged(LabelOwnerChangedEvent* e)
		{
			if (e->Update(&LabelRef))
			{
				Document->LabelOwnerChanged.HandlerFilter(HandlerOwner) = LabelRef.Segment;
				Document->LabelRemoved.HandlerFilter(HandlerRemove) = LabelRef.Segment;
			}
		}

		void HandleRemoved(LabelRemovedEvent* e)
		{
			if (e->IsRemoved(LabelRef))
			{
				LabelRef = { nullptr, 0 };
				Clear();
			}
		}

	public:
		void Set(DocumentLabelIndex label)
		{
			assert(label.Segment);
			Clear();
			Document = label.Segment->GetDocument();
			LabelRef = label;
			HandlerOwner = Document->LabelOwnerChanged
				.AddHandler(EventHandler::FromMember(HandleOwnerChanged, this), label.Segment);
			HandlerRemove = Document->LabelRemoved
				.AddHandler(EventHandler::FromMember(HandleRemoved, this), label.Segment);
		}

		DocumentLabelIndex Get()
		{
			return LabelRef;
		}

		void Clear()
		{
			if (Document)
			{
				Document->LabelOwnerChanged.RemoveHandler(HandlerOwner);
				Document->LabelRemoved.RemoveHandler(HandlerRemove);
				Document = nullptr;
			}
		}
	};
}
