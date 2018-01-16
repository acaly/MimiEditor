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

void Mimi::TextSegment::MoveLabels(TextSegment* dest, std::uint32_t begin)
{
	assert(begin > 0 || dest == GetPreviousSegment()); //split or merge
	//Move referred labels
	std::uint32_t i = FirstLabel();
	LabelData* firstRef = nullptr;
	LabelData* lastRef = nullptr;
	std::uint32_t lastLen = 0;
	while (NextLabel(&i))
	{
		LabelData* label = ReadLabelData(i);
		if (label->Position >= begin)
		{
			if (label->Type & LabelType::Referred)
			{
				if (!firstRef) firstRef = label;
				lastRef = label;
				lastLen = GetLabelLength(label);
			}
		}
	}
	TextSegment* next = GetNextSegment();
	//Move referenced labels
	if (firstRef)
	{
		//Copy data.
		std::uint32_t space = lastRef - firstRef + lastLen;
		std::uint32_t alloc = dest->AllocateLabelSpace(space);
		std::uint32_t firstIndex = GetLabelIndex(firstRef);
		std::uint32_t lastIndex = GetLabelIndex(lastRef);
		i = firstIndex;
		do
		{
			LabelData* label = ReadLabelData(i);
			if (label->Position >= begin && label->Type & LabelType::Referred)
			{
				std::uint32_t offset = i - firstIndex;
				std::uint32_t len = GetLabelLength(label);

				if (begin == 0 && label->Type & LabelType::Continuous)
				{
					//Merge the two labels as one.
					//We have ensured that dest is the previous segment.
					std::uint32_t linked = label[1].Previous;
					std::uint16_t rangeLen = label[1].Position - label[0].Position;
					LabelData* linkedLabel = dest->ReadLabelData(linked);
					assert(linkedLabel[1].Next == i);
					linkedLabel[1].Position += rangeLen;
					linkedLabel[1].Next = label[1].Next;
					if ((label->Type & LabelType::Unfinished) == 0)
					{
						linkedLabel->Type &= ~LabelType::Unfinished;
					}
				}
				else
				{
					std::memcpy(dest->ReadLabelData(alloc + offset), label, len * sizeof(LabelData));
					//Update linked next
					if (label->Type & LabelType::Unfinished)
					{
						std::uint32_t linked = label[1].Next;
						assert(next->ReadLabelData(linked)[1].Previous == i);
						next->ReadLabelData(linked)[1].Previous = alloc + offset;
					}
				}
				EraseLabelSpace(i, len);
			}
		} while (NextLabel(&i) && i <= lastIndex);
		std::int32_t change = static_cast<std::int32_t>(alloc) - static_cast<std::int32_t>(firstIndex);
		NotifyLabelOwnerChange(dest, begin, GetCurrentLength(), change);
	}
	//Move other labels.
	i = FirstLabel();
	while (NextLabel(&i))
	{
		LabelData* label = ReadLabelData(i);
		if (label->Position >= begin)
		{
			LabelData* label = ReadLabelData(i);
			std::uint32_t len = GetLabelLength(label);
			std::uint32_t alloc = dest->AllocateLabelSpace(len);

			if (begin == 0 && label->Type & LabelType::Continuous)
			{
				//Merge the two labels as one.
				//We have ensured that dest is the previous segment.
				std::uint32_t linked = label[1].Previous;
				std::uint16_t rangeLen = label[1].Position - label[0].Position;
				LabelData* linkedLabel = dest->ReadLabelData(linked);
				assert(linkedLabel[1].Next == i);
				linkedLabel[1].Position += rangeLen;
				linkedLabel[1].Next = label[1].Next;
				if ((label->Type & LabelType::Unfinished) == 0)
				{
					linkedLabel->Type &= ~LabelType::Unfinished;
				}
			}
			else
			{
				std::memcpy(dest->ReadLabelData(alloc), label, len * sizeof(LabelData));
				//Update linked next
				if (label->Type & LabelType::Unfinished)
				{
					std::uint32_t linked = label[1].Next;
					assert(next->ReadLabelData(linked)[1].Previous == i);
					next->ReadLabelData(linked)[1].Previous = alloc;
				}
			}
			EraseLabelSpace(i, len);
		}
	}
}

void Mimi::TextSegment::NotifyLabelOwnerChange(TextSegment * newOwner, std::uint32_t begin,
	std::uint32_t end, std::int32_t change)
{
	LabelOwnerChangeEvent e;
	e.OldOwner = this;
	e.NewOwner = newOwner;
	e.BeginPosition = begin;
	e.EndPosition = end;
	e.IndexChange = change;
	GetDocument()->LabelOwnerChange.InvokeAll(&e);
}
