#include "TextSegment.h"
#include "TextSegmentList.h"
#include "Document.h"

Mimi::TextSegment::TextSegment(Mimi::DynamicBuffer& buffer, bool continuous,
	bool unfinished, ModifiedFlag modified)
	: Continuous(continuous, unfinished), Modified(modified)
{
	Parent = nullptr;
	Index = 0;
	ContentBuffer = buffer.MakeStaticBuffer();
	ActiveData = nullptr;
}

Mimi::TextSegment::TextSegment(bool continuous, bool unfinished, ModifiedFlag modified)
	: Continuous(continuous, unfinished), Modified(modified)
{
	Parent = nullptr;
	Index = 0;
	ContentBuffer.Clear();
	ActiveData = nullptr;
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
}

Mimi::TextSegment* Mimi::TextSegment::GetPreviousSegment()
{
	return Parent->GetElementBefore(Index);
}

Mimi::TextSegment* Mimi::TextSegment::GetNextSegment()
{
	return Parent->GetElementAfter(Index);
}

//0: same or error, 1: s1 is after s2, -1: s1 is before s2.

int Mimi::TextSegment::ComparePosition(TextSegment * s1, TextSegment * s2)
{
	if (s1 == s2) return 0;
	TextSegmentList* l1 = s1->GetParent();
	TextSegmentList* l2 = s2->GetParent();
	return TextSegmentList::ComparePosition(l1, l2);
}

Mimi::Document* Mimi::TextSegment::GetDocument()
{
	return Parent->GetDocument();
}

std::size_t Mimi::TextSegment::GetLineIndex()
{
	std::size_t n = Parent->GetAbsLineIndex();
	for (std::uint16_t i = 0; i < Index; ++i)
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

	std::size_t length = ContentBuffer.GetSize();
	ActiveData = new ActiveTextSegmentData(ContentBuffer);
	ContentBuffer.ClearRef();

	//Setup modification tracer
	ActiveData->Modifications.Resize(GetDocument()->GetSnapshotCapacity());
	std::size_t count = GetDocument()->GetSnapshotCount();
	for (std::size_t i = 0; i < count; ++i)
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

void Mimi::TextSegment::Split(std::size_t pos, bool newLine)
{
	MakeActive();

	//Make the new segment
	TextSegment* newSegment = new TextSegment(!newLine, Continuous.IsUnfinished(), ModifiedFlag::All);
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

void Mimi::TextSegment::ReplaceText(std::size_t pos, std::size_t sel, DynamicBuffer* content)
{
	assert(IsActive());

	assert(pos < ActiveData->ContentBuffer.GetLength());
	assert(sel < ActiveData->ContentBuffer.GetLength() - pos);
	assert(content->GetLength() < MaxLength - (ActiveData->ContentBuffer.GetLength() - sel));

	std::size_t insertLen = content->GetLength();
	//Content
	ActiveData->ContentBuffer.Replace(pos, sel, content->GetRawData(), insertLen);
	//Modification tracer
	ActiveData->Modifications.Delete(pos, sel);
	ActiveData->Modifications.Insert(pos, insertLen);
	//Labels
	std::size_t i = FirstLabel();
	std::size_t totalLen = GetCurrentLength();
	while (NextLabel(&i))
	{
		LabelData* label = ReadLabelData(i);
		switch (label->Type & LabelType::Topology)
		{
		case LabelType::Point:
		{
			std::uint16_t posp = label->Position;
			if (posp >= pos && posp < pos + sel)
			{
				switch (label->Type & LabelType::Alignment)
				{
				case LabelType::Left:
					label->Position = static_cast<std::uint16_t>(pos + insertLen);
					break;
				case LabelType::Right:
					label->Position = static_cast<std::uint16_t>(pos);
					break;
				case LabelType::Center:
					NotifyLabelRemoved(i);
					EraseLabelSpace(i, GetLabelLength(label));
					break;
				default:
					assert(!"Unknown label alignment.");
				}
			}
		}
		case LabelType::Range:
		{
			std::uint16_t pos1 = label[0].Position;
			std::uint16_t pos2 = label[1].Position;
			if (pos1 >= pos && pos2 < pos + sel)
			{
				//Remove
				NotifyLabelRemoved(i);
				EraseLabelSpace(i, GetLabelLength(label));
			}
			else
			{
				//Update two sides
				if (pos1 >= pos && pos1 < pos + sel && !(label->Type & LabelType::Continuous))
				{
					label[0].Position = static_cast<std::uint16_t>(pos + insertLen);
				}
				if (pos2 >= pos && pos2 < pos + sel)
				{
					label[1].Position = static_cast<std::uint16_t>(pos);
				}
				if (label->Type & LabelType::Unfinished)
				{
					label[1].Position = static_cast<std::uint16_t>(totalLen - 1);
				}
			}
			break;
		}
		case LabelType::Line:
			//Do nothing
			break;
		default:
			assert(!"Unknown label type.");
		}
	}
	GetParent()->OnElementDataChanged();
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
	std::size_t cap = GetDocument()->GetSnapshotCapacity();
	std::size_t count = GetDocument()->GetSnapshotCount();
	assert(cap >= count);
	if (IsActive())
	{
		if (resize)
		{
			ActiveData->Modifications.Resize(cap);
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

void Mimi::TextSegment::DisposeSnapshot(std::size_t num, bool resize)
{
	if (IsActive())
	{
		std::size_t newNum = GetDocument()->GetSnapshotCount();
		ActiveData->Modifications.DisposeSnapshot(newNum + num, newNum);
		if (resize)
		{
			ActiveData->Modifications.Resize(GetDocument()->GetSnapshotCapacity());
		}
	}
}

std::size_t Mimi::TextSegment::ConvertSnapshotPosition(std::size_t snapshot, std::size_t pos, int dir)
{
	if (IsActive())
	{
		return ActiveData->Modifications.ConvertFromSnapshot(snapshot, pos, dir);
	}
	return pos;
}

void Mimi::TextSegment::MoveLabels(TextSegment* dest, std::size_t begin)
{
	assert(begin > 0 || dest == GetPreviousSegment()); //split or merge
	//Move referred labels
	std::size_t i = FirstLabel();
	LabelData* firstRef = nullptr;
	LabelData* lastRef = nullptr;
	std::size_t lastLen = 0;
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
		std::size_t space = lastRef - firstRef + lastLen;
		std::size_t alloc = dest->AllocateLabelSpace(space);
		std::size_t firstIndex = GetLabelIndex(firstRef);
		std::size_t lastIndex = GetLabelIndex(lastRef);
		i = firstIndex;
		do
		{
			LabelData* label = ReadLabelData(i);
			if (label->Position >= begin && label->Type & LabelType::Referred)
			{
				std::size_t offset = i - firstIndex;
				std::size_t len = GetLabelLength(label);

				if (begin == 0 && label->Type & LabelType::Continuous)
				{
					//Merge the two labels as one.
					//We have ensured that dest is the previous segment.
					std::uint16_t linked = label[1].Previous;
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
						std::uint16_t linked = label[1].Next;
						assert(next->ReadLabelData(linked)[1].Previous == i);
						next->ReadLabelData(linked)[1].Previous = static_cast<std::uint16_t>(alloc + offset);
					}
				}
				EraseLabelSpace(i, len);
			}
		} while (NextLabel(&i) && i <= lastIndex);
		std::ptrdiff_t change = static_cast<std::ptrdiff_t>(alloc) - static_cast<std::ptrdiff_t>(firstIndex);
		NotifyLabelOwnerChanged(dest, begin, GetCurrentLength(), change);
	}
	//Move other labels.
	i = FirstLabel();
	while (NextLabel(&i))
	{
		LabelData* label = ReadLabelData(i);
		if (label->Position >= begin)
		{
			LabelData* label = ReadLabelData(i);
			std::ptrdiff_t len = GetLabelLength(label);
			std::ptrdiff_t alloc = dest->AllocateLabelSpace(len);

			if (begin == 0 && label->Type & LabelType::Continuous)
			{
				//Merge the two labels as one.
				//We have ensured that dest is the previous segment.
				std::uint16_t linked = label[1].Previous;
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
					std::uint16_t linked = label[1].Next;
					assert(next->ReadLabelData(linked)[1].Previous == i);
					next->ReadLabelData(linked)[1].Previous = static_cast<std::uint16_t>(alloc);
				}
			}
			EraseLabelSpace(i, len);
		}
	}
}

void Mimi::TextSegment::NotifyLabelOwnerChanged(TextSegment * newOwner, std::size_t begin,
	std::size_t end, std::ptrdiff_t change)
{
	LabelOwnerChangedEvent e;
	e.OldOwner = this;
	e.NewOwner = newOwner;
	e.BeginPosition = begin;
	e.EndPosition = end;
	e.IndexChange = change;
	GetDocument()->LabelOwnerChanged.InvokeAll(&e);
}

void Mimi::TextSegment::NotifyLabelRemoved(std::size_t index)
{
	LabelRemovedEvent e;
	e.Owner = this;
	e.Index = index;
	GetDocument()->LabelRemoved.InvokeAll(&e);
}
