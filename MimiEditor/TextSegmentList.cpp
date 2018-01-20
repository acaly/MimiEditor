#include "TextSegmentList.h"
#include "TextSegment.h"

Mimi::TextSegment * Mimi::TextSegmentTree::GetFirstSegment()
{
	TextSegmentList* node = Root;
	while (!node->IsLeaf)
	{
		node = node->DataAsNode()[0];
	}
	return node->DataAsElement()[0];
}

Mimi::TextSegment * Mimi::TextSegmentTree::GetSegmentWithLineIndex(std::size_t index)
{
	TextSegmentList* node = Root;
	std::size_t count = 0;
	while (!node->IsLeaf)
	{
		TextSegmentList** ptr = node->DataAsNode();
		while (count + (*ptr)->LineCount < index)
		{
			count = count + (*ptr)->LineCount;
			ptr += 1;
			assert(*ptr);
		}
		node = *ptr;
	}
	TextSegment** ptrs = node->DataAsElement();
	while (count < index)
	{
		if (!(*ptrs)->IsContinuous())
		{
			count += 1;
		}
		ptrs += 1;
		assert(*ptrs);
	}
	return *ptrs;
}

Mimi::DocumentPositionI Mimi::TextSegmentTree::ConvertDocumentI(DocumentPositionS s)
{
	TextSegment* seg = s.Segment;
	std::size_t count = s.Position;
	std::size_t line = seg->GetLineIndex();
	while (seg->IsContinuous())
	{
		seg = seg->GetPreviousSegment();
		count += seg->GetCurrentLength();
	}
	return { line, count };
}

Mimi::DocumentPositionS Mimi::TextSegmentTree::ConvertDocumentS(DocumentPositionI i)
{
	TextSegment* seg = GetSegmentWithLineIndex(i.Line);
	std::size_t count = i.Position;
	while (count < seg->GetCurrentLength())
	{
		assert(seg->IsContinuous());
		count -= seg->GetCurrentLength();
		seg = seg->GetNextSegment();
	}
	return { seg, count };
}
