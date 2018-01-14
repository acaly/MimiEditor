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

	std::uint32_t length = ContentBuffer.GetSize();
	ActiveData = new ActiveTextSegmentData(ContentBuffer);
	ContentBuffer.ClearRef();

	//Setup modification tracer
	ActiveData->Modifications.Resize(GetDocument()->GetSnapshotCapacity());
	std::uint32_t count = GetDocument()->GetSnapshotCount();
	for (std::uint32_t i = 0; i < count; ++i)
	{
		ActiveData->Modifications.NewSnapshot(i + 1, length);
	}
}

void Mimi::TextSegment::MakeInactive()
{
	if (!IsActive()) return;
	assert(GetDocument()->GetSnapshotCount() == 0);
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

void Mimi::TextSegment::CheckAndMakeInactive(std::uint32_t time)
{
	if (IsActive() && ActiveData->LastModifiedTime < time &&
		GetDocument()->GetSnapshotCount() == 0)
	{
		MakeInactive();
	}
}

Mimi::StaticBuffer Mimi::TextSegment::MakeSnapshot(bool resize)
{
	//Start tracing
	std::uint32_t cap = GetDocument()->GetSnapshotCapacity();
	std::uint32_t count = GetDocument()->GetSnapshotCount();
	assert(cap >= count);
	if (IsActive())
	{
		if (resize)
		{
			ActiveData->Modifications.Resize(count);
		}
		ActiveData->Modifications.NewSnapshot(count, ActiveData->ContentBuffer.GetLength());
	}

	//Make a buffer
	if (!IsActive())
	{
		return ContentBuffer.NewRef();
	}
	else if (!Modified.SinceSnapshot() && !ActiveData->SnapshotCache.IsNull())
	{
		return ActiveData->SnapshotCache.NewRef();
	}
	else
	{
		StaticBuffer ret = ActiveData->ContentBuffer.MakeStaticBuffer();
		ActiveData->SnapshotCache.TryClearRef();
		ActiveData->SnapshotCache = ret;
		Modified.ClearSnapshot();
		return ret.NewRef();
	}
}

void Mimi::TextSegment::DisposeSnapshot(std::uint32_t num, bool resize)
{
	if (IsActive())
	{
		std::uint32_t newNum = GetDocument()->GetSnapshotCount();
		ActiveData->Modifications.DisposeSnapshot(newNum + num, newNum);
		if (resize)
		{
			ActiveData->Modifications.Resize(GetDocument()->GetSnapshotCapacity());
		}
	}
}

std::uint32_t Mimi::TextSegment::ConvertSnapshotPosition(std::uint32_t snapshot, std::uint32_t pos, int dir)
{
	if (IsActive())
	{
		return ActiveData->Modifications.ConvertFromSnapshot(snapshot, pos, dir);
	}
	return pos;
}

bool Mimi::TextSegment::FindLinkedLabelWithPrevious(std::uint32_t index, std::uint32_t * result)
{
	std::uint32_t i = FirstLabel();
	while (NextLabel(&i))
	{
		LabelData* label = ReadLabelData(i);
		if (label->Type & LabelType::Continuous)
		{
			if (label[1].Previous == index)
			{
				*result = i;
				return true;
			}
		}
	}
	return false;
}

bool Mimi::TextSegment::FindLinkedLabelWithNext(std::uint32_t index, std::uint32_t * result)
{
	std::uint32_t i = FirstLabel();
	while (NextLabel(&i))
	{
		LabelData* label = ReadLabelData(i);
		if (label->Type & LabelType::Unfinished)
		{
			if (label[1].Next == index)
			{
				*result = i;
				return true;
			}
		}
	}
	return false;
}

void Mimi::TextSegment::NotifyLabelOwnerChange(TextSegment * newOwner, std::uint32_t begin, std::uint32_t end)
{
	LabelOwnerChangeEvent e;
	e.OldOwner = this;
	e.NewOwner = newOwner;
	e.BeginPosition = begin;
	e.EndPosition = end;
	GetDocument()->LabelOwnerChange.InvokeAll(&e);
}
