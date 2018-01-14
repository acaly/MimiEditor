#include "TextSegment.h"
#include "TextSegmentList.h"
#include "Document.h"

Mimi::TextSegment::TextSegment(Mimi::DynamicBuffer& buffer, bool continuous, bool unfinished)
	: Continuous(continuous, unfinished)
{
	Parent = nullptr;
	Index = 0;
	ContentBuffer = buffer.MakeStaticBuffer();
	ActiveData = nullptr;
	RenderCache = nullptr;
}

Mimi::TextSegment::TextSegment(bool continuous, bool unfinished)
	: Continuous(continuous, unfinished)
{
	Parent = nullptr;
	Index = 0;
	ContentBuffer.Clear();
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

void Mimi::TextSegment::MakeActive()
{
	if (IsActive()) return;
	ActiveData = new ActiveTextSegmentData(ContentBuffer);
	ContentBuffer.ClearRef();
}

void Mimi::TextSegment::MakeInactive()
{
	if (!IsActive()) return;
	ContentBuffer = ActiveData->ContentBuffer.MakeStaticBuffer();
	delete ActiveData;
	ActiveData = nullptr;
}

void Mimi::TextSegment::Split(std::uint32_t pos, bool newLine)
{
	MakeActive();

	//Make the new segment
	TextSegment* newSegment = new TextSegment(!newLine, Continuous.IsUnfinished());
	Continuous.SetUnfinished(!newLine);
	newSegment->ActiveData = new ActiveTextSegmentData();

	//Content
	ActiveData->ContentBuffer.SplitRight(newSegment->ActiveData->ContentBuffer, pos);
	//Modification
	newSegment->ActiveData->Modifications.Resize(GetDocument()->GetSnapshotCapacity());
	ActiveData->Modifications.SplitInto(newSegment->ActiveData->Modifications,
		GetDocument()->GetSnapshotCount(), pos);
	//Label
	LabelSplit(newSegment, pos);

	//Add to list
	Parent->InsertElement(Index + 1, newSegment);
}

void Mimi::TextSegment::Merge()
{
	TextSegment* other = GetNextSegment();

	MakeActive();
	other->MakeActive();

	//Content
	ActiveData->ContentBuffer.Insert(ActiveData->ContentBuffer.GetLength(),
		other->ActiveData->ContentBuffer.GetRawData(),
		other->ActiveData->ContentBuffer.GetLength());
	//Modification
	other->ActiveData->Modifications.Resize(GetDocument()->GetSnapshotCapacity());
	ActiveData->Modifications.MergeWith(other->ActiveData->Modifications,
		GetDocument()->GetSnapshotCount());
	//Label
	LabelMerge(other);

	//Dispose
	other->ActiveData->SnapshotCache.ClearRef();
	other->ContentBuffer.ClearRef();
	other->Labels.Clear();
	other->Parent->RemoveElement(other->Index);
	delete other;
}
