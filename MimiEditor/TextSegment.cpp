#include "TextSegment.h"
#include "TextSegmentList.h"
#include "TextDocument.h"
#include "CodePage.h"

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
		delete ActiveData;
	}
	ContentBuffer.TryClearRef();
	Labels.Clear();
}

void Mimi::TextSegment::AddToList(TextSegmentList* list, std::size_t index)
{
	assert(index < TextSegmentTreeFactor);
	assert(Parent == nullptr);
	assert(list != nullptr);
	Parent = list;
	Index = static_cast<std::uint16_t>(index);
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

Mimi::TextDocument* Mimi::TextSegment::GetDocument()
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
	ActiveData = new ActiveTextSegmentData(ContentBuffer.MoveRef());

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

Mimi::TextSegment* Mimi::TextSegment::Split(std::size_t pos, bool newLine)
{
	MakeActive();

	if (newLine && pos == GetCurrentLength() && IsUnfinished())
	{
		//Note that we don't update data here, as it will be done after spliting.
		TextSegment* next = GetNextSegment();
		assert(next && next->IsContinuous());
		Continuous.SetUnfinished(false);
		next->Continuous.SetContinuous(false);
		return next;
	}
	if (newLine && pos == 0 && IsContinuous())
	{
		//Note that we don't update data here, as it will be done after spliting.
		TextSegment* prev = GetPreviousSegment();
		assert(prev && prev->IsUnfinished());
		Continuous.SetContinuous(false);
		prev->Continuous.SetUnfinished(false);
		return prev;
	}

	//Make the new segment
	TextSegment* newSegment = new TextSegment(!newLine, Continuous.IsUnfinished(), ModifiedFlag::All);
	Continuous.SetUnfinished(!newLine);
	newSegment->ActiveData = new ActiveTextSegmentData();

	newSegment->ActiveData->LastModifiedTime = ActiveData->LastModifiedTime;

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

	return newSegment;
}

void Mimi::TextSegment::Merge()
{
	TextSegment* other = GetNextSegment();
	assert(other);

	MakeActive();
	other->MakeActive();

	Modified.Modify();
	other->Modified.Modify();
	ActiveData->LastModifiedTime =
		ActiveData->LastModifiedTime > other->ActiveData->LastModifiedTime ?
		ActiveData->LastModifiedTime : other->ActiveData->LastModifiedTime;

	//Check size
	if (GetCurrentLength() + other->GetCurrentLength() > MaxLength)
	{
		Continuous.SetUnfinished(true);
		other->Continuous.SetContinuous(true);
		return;
	}

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

void Mimi::TextSegment::ReplaceText(std::size_t pos, std::size_t sel, DynamicBuffer* content, std::size_t globalPosition)
{
	MakeActive();

	assert(pos <= ActiveData->ContentBuffer.GetLength());
	assert(sel <= ActiveData->ContentBuffer.GetLength() - pos);
	assert(content == nullptr ||
		content->GetLength() < MaxLength - (ActiveData->ContentBuffer.GetLength() - sel));

	std::size_t insertLen = content ? content->GetLength() : 0;

	//Content
	ActiveData->ContentBuffer.Replace(pos, sel, content ? content->GetRawData() : nullptr, insertLen);
	//Modification tracer
	if (GetDocument()->GetSnapshotCount()) //TODO create a snapshot at the beginning?
	{
		ActiveData->Modifications.Delete(pos, sel);
		ActiveData->Modifications.Insert(pos, insertLen);
	}
	//Labels
	UpdateLabels(pos, sel, insertLen, globalPosition);
	//Event
	GetParent()->OnElementDataChanged(); //TODO Called at document level API?
}

void Mimi::TextSegment::CheckAndMakeInactive(std::uint32_t time)
{
	if (IsActive() && ActiveData->LastModifiedTime < time &&
		GetDocument()->GetSnapshotCount() == 0)
	{
		MakeInactive();
	}
}

void Mimi::TextSegment::EnsureInsertionSize(std::size_t pos, std::size_t size,
	DocumentPositionS* before, DocumentPositionS* after)
{
	assert(size < MaxLength);
	if (GetCurrentLength() + size < MaxLength)
	{
		if (before)
		{
			*before = { this, pos };
		}
		if (after)
		{
			*after = { this, pos + size };
		}
	}
	else if (pos + size < MaxLength)
	{
		TextSegment* next = Split(pos, false); //This will not produce empty segment.
		if (before)
		{
			*before = { this, pos };
		}
		if (after)
		{
			*after = { next, 0 };
		}
	}
	else
	{
		TextSegment* next2 = Split(pos, false); //This will not produce empty segment.
		TextSegment* next = Split(pos, false); //A temporary empty segment.
		if (before)
		{
			*before = { next, 0 };
		}
		if (after)
		{
			*after = { next2, 0 };
		}
	}
}

static char32_t GetLastChar(const Mimi::mchar8_t* bufferEnd, Mimi::CodePage cp)
{
	std::size_t len = cp.GetNormalWidth();
	const Mimi::mchar8_t* ch = bufferEnd - len;
	char32_t ret;
	if (cp.CharToUTF32(ch, &ret) != len)
	{
		return 0;
	}
	return ret;
}

bool Mimi::TextSegment::HasLineBreak()
{
	if (GetCurrentLength() == 0)
	{
		return false;
	}
	const Mimi::mchar8_t* bufferEnd;
	if (IsActive())
	{
		bufferEnd = &ActiveData->ContentBuffer.GetRawData()[ActiveData->ContentBuffer.GetLength()];
	}
	else
	{
		bufferEnd = &ContentBuffer.GetRawData()[ContentBuffer.GetSize()];
	}
	return GetLastChar(bufferEnd, GetDocument()->TextEncoding) == '\n';
}

void Mimi::TextSegment::CheckLineBreak()
{
	bool check = HasLineBreak();
	if (!IsUnfinished() && check)
	{
		if (GetNextSegment())
		{
			Merge();
		}
	}
	else if (IsUnfinished() && !check)
	{
		Continuous.SetUnfinished(false);
		TextSegment* other = GetNextSegment();
		if (other)
		{
			other->Continuous.SetContinuous(false);
		}
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

std::size_t Mimi::TextSegment::GetHistoryLength(std::size_t snapshot)
{
	if (IsActive())
	{
		return ActiveData->Modifications.GetSnapshotLength(snapshot);
	}
	return GetCurrentLength();
}

void Mimi::TextSegment::UpdateRangeLabel(std::size_t i, std::size_t pos, std::size_t sel,
	std::size_t insertLen, std::size_t globalPosition)
{
	LabelData* label = ReadLabelData(i);

	std::uint16_t pos1 = label[0].Position;
	std::uint16_t pos2 = label[1].Position;
	if (pos1 >= pos && pos2 < pos + sel)
	{
		bool continuous = label->Type & LabelType::Continuous;
		bool unfinished = label->Type & LabelType::Unfinished;
		if (continuous && unfinished)
		{
			//Update link.
			std::uint16_t prev = label[1].Previous;
			std::uint16_t next = label[1].Next;
			assert(GetNextSegment() && GetPreviousSegment());
			LabelData* lprev = GetPreviousSegment()->ReadLabelData(prev);
			LabelData* lnext = GetNextSegment()->ReadLabelData(next);
			assert(lprev[1].Next == i);
			assert(lnext[1].Previous == i);
			lprev[1].Next = next;
			lnext[1].Previous = prev;
		}
		else if (continuous)
		{
			//Update previous.
			std::uint16_t prev = label[1].Previous;
			assert(GetPreviousSegment());
			LabelData* lprev = GetPreviousSegment()->ReadLabelData(prev);
			assert(lprev[1].Next == i);
			lprev->Type &= ~LabelType::Unfinished;
		}
		else if (unfinished)
		{
			//Update next and notify owner change.
			std::uint16_t next = label[1].Next;
			assert(GetNextSegment());
			LabelData* lnext = GetNextSegment()->ReadLabelData(next);
			assert(lnext[1].Previous == i);
			lnext->Type &= ~LabelType::Continuous;
			NotifyLabelOwnerChanged(GetNextSegment(), i, next);
		}
		else
		{
			//Just remove
			if (label->Type & LabelType::Referred)
			{
				NotifyLabelRemoved(i, globalPosition);
			}
		}
		EraseLabelSpace(i, GetLabelLength(label));
	}
	else
	{
		//Update one of the two sides.
		if (pos1 >= pos && pos1 < pos + sel)
		{
			if (!(label->Type & LabelType::Continuous))
			{
				label[0].Position = static_cast<std::uint16_t>(pos + insertLen);
			} //else: keep it at 0.
			label[1].Position -= static_cast<std::uint16_t>(sel);
			label[1].Position += static_cast<std::uint16_t>(insertLen);
		}
		else if (pos2 >= pos && pos2 < pos + sel)
		{
			if (label->Type & LabelType::Unfinished)
			{
				std::size_t totalLen = GetCurrentLength();
				label[1].Position = static_cast<std::uint16_t>(totalLen - 1);
			}
			else
			{
				label[1].Position = static_cast<std::uint16_t>(pos);
			}
		}
	}
}

void Mimi::TextSegment::UpdateLabels(std::size_t pos, std::size_t sel, std::size_t insertLen, std::size_t globalPosition)
{
	std::size_t i = FirstLabel();
	std::size_t totalLen = GetCurrentLength();

	//Special case: insert to temporary empty segment, produced by:
	//1. EnsureInsertionSize & TextDocument::Insert
	//2. InsertLineBreak & TextDocument::Insert
	if (sel == 0 && totalLen == insertLen)
	{
		assert(pos == 0);
		while (NextLabel(&i))
		{
			LabelData* label = ReadLabelData(i);
			assert((label->Type & LabelType::Topology) == LabelType::Range);
			assert(label->Type & LabelType::Continuous);
			assert(label[0].Position == 0);
			assert(label[1].Position + 1 == 0);
			label[1].Position = static_cast<std::uint16_t>(totalLen - 1);
		}
		return;
	}

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
					if (label->Type & LabelType::Referred)
					{
						NotifyLabelRemoved(i, globalPosition);
					}
					EraseLabelSpace(i, GetLabelLength(label));
					break;
				default:
					assert(!"Unknown label alignment.");
				}
			}
		}
		case LabelType::Range:
		{
			UpdateRangeLabel(i, pos, sel, insertLen, globalPosition);
			break;
		}
		case LabelType::Line:
			//Do nothing
			break;
		default:
			assert(!"Unknown label type.");
		}
	}
}

void Mimi::TextSegment::UpdateLabelsDeleteAll(TextSegment* moveBack, TextSegment* moveForward, std::size_t globalPosition)
{
	TextSegment* target = GetNextSegment();
	if (!IsContinuous() && IsUnfinished())
	{
		assert(target && target->IsContinuous());
	}

	std::size_t currentLen = GetCurrentLength();

	std::size_t i = FirstLabel();
	while (NextLabel(&i))
	{
		LabelData* label = ReadLabelData(i);
		switch (label->Type & LabelType::Topology)
		{
		case LabelType::Line:
			if (!IsContinuous() && IsUnfinished())
			{
				//Line label in unfinished line is not that many. We can just move one by one.
				std::size_t labelLen = GetLabelLength(label);
				std::size_t newIndex = target->AllocateLabelSpace(labelLen);
				std::memcpy(target->ReadLabelData(newIndex), label, sizeof(LabelData) * labelLen);
				NotifyLabelOwnerChanged(target, i, newIndex);
			}
			else
			{
				if (label->Type & LabelType::Referred)
				{
					NotifyLabelRemoved(i, globalPosition);
				}
			}
			break;
		case LabelType::Point:
			TextSegment* dest;
			switch (label->Type & LabelType::Alignment)
			{
			case LabelType::Center:
				if (label->Type & LabelType::Referred)
				{
					NotifyLabelRemoved(i, globalPosition);
				}
				//No need to move the label. Continue the loop.
				continue;
			case LabelType::Left:
				dest = moveForward;
				break;
			case LabelType::Right:
				dest = moveBack;
				break;
			default:
				assert(!"Invalid label direction.");
				continue; //Never get here. Avoid using dest.
			}
			if (dest)
			{
				std::size_t len = GetLabelLength(label);
				std::size_t newIndex = dest->AllocateLabelSpace(len);
				std::memcpy(dest->ReadLabelData(newIndex), label, len);
				NotifyLabelOwnerChanged(dest, i, newIndex);
			}
			break;
		case LabelType::Range:
			UpdateRangeLabel(i, 0, currentLen, 0, globalPosition);
			break;
		default:
			assert(!"Invalid label type.");
			break;
		}
	}
}

void Mimi::TextSegment::MoveLabels(TextSegment* dest, std::size_t begin)
{
	assert((begin > 0 || GetCurrentLength() == 0) || dest == GetPreviousSegment()); //split or merge
	//Calculate required space for referred labels.
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
	//Move referred labels.
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
			if (label->Position >= begin && (label->Type & LabelType::Referred))
			{
				std::size_t offset = i - firstIndex;
				std::size_t len = GetLabelLength(label);

				if (begin == 0 && (label->Type & LabelType::Continuous))
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
			}
		} while (NextLabel(&i) && i <= lastIndex);
		std::ptrdiff_t change = static_cast<std::ptrdiff_t>(alloc) - static_cast<std::ptrdiff_t>(firstIndex);
		NotifyLabelOwnerChanged(dest, begin, GetCurrentLength(), change);
	}
	//1. Move non-referred labels.
	//2. Clear referred labels.
	//3. Split range label (which starts before begin, referred or non-referred).
	i = FirstLabel();
	while (NextLabel(&i))
	{
		LabelData* label = ReadLabelData(i);
		if (label->Position >= begin)
		{
			LabelData* label = ReadLabelData(i);
			std::ptrdiff_t len = GetLabelLength(label);

			if (label->Type & LabelType::Referred)
			{
				EraseLabelSpace(i, len);
				continue;
			}

			std::size_t alloc = dest->AllocateLabelSpace(len);

			if (begin == 0 && (label->Type & LabelType::Continuous))
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
		else if ((label->Type & LabelType::Topology) == LabelType::Range && label[1].Position >= begin)
		{
			//Split label
			std::size_t splitLabelAlloc = dest->AllocateLabelSpace(2);
			LabelData* splitLabelNext = dest->ReadLabelData(splitLabelAlloc);

			//Create new
			splitLabelNext->Type = label->Type;
			splitLabelNext->Type |= LabelType::Continuous;
			splitLabelNext->Handler = label->Handler;
			splitLabelNext->Position = 0;
			//Note that split at the end of a segment is allowed (temporarily).
			//This may give end position of -1 (0xFFFF), which will be updated when we insert back.
			splitLabelNext[1].Position = static_cast<std::uint16_t>(label[1].Position - begin);

			//Update old
			label[1].Position = static_cast<std::uint16_t>(begin - 1);
			label->Type |= LabelType::Unfinished;

			//Link the two
			std::uint16_t linked = label[1].Next;
			label[1].Next = static_cast<std::uint16_t>(splitLabelAlloc);
			splitLabelNext[1].Previous = static_cast<std::uint16_t>(i);

			//Update next, if any
			if (splitLabelNext->Type & LabelType::Unfinished)
			{
				assert(next->ReadLabelData(linked)[1].Previous == i);
				next->ReadLabelData(linked)[1].Previous = static_cast<std::uint16_t>(splitLabelAlloc);
			}
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
	e.SingleId = SIZE_MAX; //Id is in uint16_t, single id test always fails.
	GetDocument()->LabelOwnerChanged.InvokeAll(&e);
}

void Mimi::TextSegment::NotifyLabelOwnerChanged(TextSegment * newOwner, std::size_t id, std::size_t newId)
{
	LabelOwnerChangedEvent e;
	e.OldOwner = this;
	e.NewOwner = newOwner;
	e.BeginPosition = 1; //begin > end, so that range test always fails.
	e.EndPosition = 0;
	e.IndexChange = static_cast<std::ptrdiff_t>(newId) - static_cast<std::ptrdiff_t>(id);
	e.SingleId = id;
	GetDocument()->LabelOwnerChanged.InvokeAll(&e);
}

void Mimi::TextSegment::NotifyLabelRemoved(std::size_t index, std::size_t globalPosition)
{
	LabelRemovedEvent e;
	e.Owner = this;
	e.Index = index;
	e.GlobalPosition = globalPosition;
	GetDocument()->LabelRemoved.InvokeAll(&e);
}
