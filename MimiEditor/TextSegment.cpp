#include "TextSegment.h"
#include "TextSegmentList.h"

Mimi::TextSegment::TextSegment(Mimi::DynamicBuffer& buffer, bool continuous, bool unfinished)
	: Continuous(continuous, unfinished)
{
	Parent = nullptr;
	Index = 0;
	ContentBuffer = buffer.MakeStaticBuffer();
	ActiveData = nullptr;
	RenderCache = nullptr;
}

Mimi::TextSegment::~TextSegment()
{
	if (ActiveData)
	{
		assert(ActiveData->SnapshotCache.IsNull());
		delete ActiveData;
	}
	assert(ContentBuffer.IsNull());
	assert(Labels.GetCount() == 0);
	assert(RenderCache == nullptr); //TODO
}

Mimi::TextSegment* Mimi::TextSegment::GetPreviousSegment()
{
	return Parent->GetElementBefore(Index);
}

Mimi::TextSegment* Mimi::TextSegment::GetNextSegment()
{
	return Parent->GetElementAfter(Index);
}

Mimi::Document* Mimi::TextSegment::GetDocument()
{
	return Parent->GetDocument();
}

std::uint32_t Mimi::TextSegment::GetLineNumber()
{
	std::uint32_t n = Parent->GetAbsLineIndex();
	for (std::uint32_t i = 0; i < Index; ++i)
	{
		if (!Parent->DataAsElement()[i]->IsContinuous())
		{
			n += 1;
		}
	}
	return n;
}

void Mimi::TextSegment::MakeDynamic()
{
	ActiveData = new ActiveTextSegmentData(ContentBuffer);
	ContentBuffer.ClearRef();
}

void Mimi::TextSegment::MakeStatic()
{
	ContentBuffer = ActiveData->ContentBuffer.MakeStaticBuffer();
	delete ActiveData;
	ActiveData = nullptr;
}

void Mimi::TextSegment::SplitLeft(std::uint32_t pos)
{

}

void Mimi::TextSegment::SplitRight(std::uint32_t pos)
{

}

void Mimi::TextSegment::MergeWithLeft()
{

}

void Mimi::TextSegment::MergeWithRight()
{

}
