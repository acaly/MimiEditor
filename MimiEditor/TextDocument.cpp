#include "TextDocument.h"
#include "Snapshot.h"
#include "TextSegment.h"
#include "FileTypeDetector.h"

Mimi::TextDocument::~TextDocument()
{
	assert(SnapshotInUse.GetCount() == 0);
	//Let the TextSegmentTree delete itself.
}

Mimi::Snapshot* Mimi::TextDocument::CreateSnapshot()
{
	bool resize = false;
	if (++SnapshotCount > SnapshotCapacity)
	{
		SnapshotCapacity *= 2;
		resize = true;
	}
	SnapshotInUse.Insert(0, true);

	Snapshot* s = new Snapshot(this, NextSnapshotIndex++);

	TextSegment* segment = this->SegmentTree.GetFirstSegment();
	assert(segment);

	s->DataLength = 0;
	do
	{
		StaticBuffer buffer = segment->MakeSnapshot(resize);
		s->DataLength += buffer.GetSize();
		s->AppendBuffer(buffer.MoveRef());
		segment = segment->GetNextSegment();
	} while (segment);

	return s;
}

void Mimi::TextDocument::DisposeSnapshot(Snapshot* s)
{
	assert(static_cast<TextDocument*>(s->GetDocument()) == this);
	std::size_t id = ConvertSnapshotToIndex(s->GetHistoryIndex());
	const bool resize = false; //Never resize

	//Update in use array.
	SnapshotInUse[id] = false;
	std::size_t lastUsed;
	for (lastUsed = SnapshotInUse.GetCount() - 1; lastUsed-- > 0; )
	{
		if (SnapshotInUse[lastUsed]) break;
	}
	std::size_t disposeNum = SnapshotInUse.GetCount() - lastUsed - 1;
	SnapshotInUse.RemoveRange(lastUsed + 1, disposeNum);

	//Dispose on each segment.
	TextSegment* segment = SegmentTree.GetFirstSegment();
	assert(segment);
	do
	{
		segment->DisposeSnapshot(lastUsed + 1, resize);
		segment = segment->GetNextSegment();
	} while (segment);

	s->ClearBuffer();
}

Mimi::DocumentLabelIndex Mimi::TextDocument::AddPointLabel(std::uint16_t handler, 
	DocumentPositionS pos, int direction, bool longData, bool referred)
{
	std::size_t len = longData ? 2 : 1;
	std::size_t id = pos.Segment->AllocateLabelSpace(len);
	LabelData* label = pos.Segment->ReadLabelData(id);
	label->Type = LabelType::Point;
	if (longData)
	{
		label->Type |= LabelType::Long;
		label[1].Additional = 0;
	}
	switch (direction)
	{
	case -1:
		label->Type |= LabelType::Left;
		break;
	case 0:
		label->Type |= LabelType::Center;
		break;
	case 1:
		label->Type |= LabelType::Right;
		break;
	default:
		assert(!"Invalid label direction.");
		break;
	}
	if (referred) label->Type |= LabelType::Referred;
	label->Handler = handler;
	label->Data = 0;
	label->Position = static_cast<std::uint16_t>(pos.Position);
	return { pos.Segment, id };
}

Mimi::DocumentLabelIndex Mimi::TextDocument::AddLineLabel(std::uint16_t handler,
	DocumentPositionS pos, bool longData, bool referred)
{
	//Add to the start of the line.
	TextSegment* s = pos.Segment;
	while (s->IsContinuous())
	{
		s = s->GetPreviousSegment();
		assert(s);
	}

	std::size_t len = longData ? 2 : 1;
	std::size_t id = s->AllocateLabelSpace(len);
	LabelData* label = s->ReadLabelData(id);
	label->Type = LabelType::Line;
	if (longData)
	{
		label->Type |= LabelType::Long;
		label[1].Additional = 0;
	}
	if (referred) label->Type |= LabelType::Referred;
	label->Handler = handler;
	label->Data = 0;
	label->Position = 0;
	return { s, id };
}

Mimi::DocumentLabelIndex Mimi::TextDocument::AddRangeLabel(std::uint16_t handler,
	DocumentPositionS begin, DocumentPositionS end, bool longData, bool referred)
{
	assert(static_cast<TextDocument*>(begin.Segment->GetDocument()) == this);
	assert(static_cast<TextDocument*>(end.Segment->GetDocument()) == this);
	assert(begin.Segment == end.Segment && begin.Position <= end.Position ||
		TextSegment::ComparePosition(begin.Segment, end.Segment) < 0);
	assert(begin.Position < begin.Segment->GetCurrentLength());
	assert(end.Position < end.Segment->GetCurrentLength());

	TextSegment* retSegment = begin.Segment;
	TextSegment* s = retSegment;
	std::size_t retId = s->AllocateLabelSpace(longData ? 3 : 2);
	std::size_t id = retId;
	LabelData* label = s->ReadLabelData(id);
	label->Type = LabelType::Range;
	if (longData)
	{
		label->Type |= LabelType::Long;
		label[2].Additional = 0;
	}
	if (referred) label->Type |= LabelType::Referred;

	while (s != end.Segment)
	{
		TextSegment* nextSegment = s->GetNextSegment();
		assert(nextSegment);
		std::size_t nextId = nextSegment->AllocateLabelSpace(2);
		LabelData* nextLabel = nextSegment->ReadLabelData(nextId);
		
		label->Type |= LabelType::Unfinished;
		label[1].Next = static_cast<std::uint16_t>(nextId);
		label[1].Position = static_cast<std::uint16_t>(s->GetCurrentLength() - 1);

		nextLabel->Type = LabelType::Range | LabelType::Continuous;
		nextLabel->Handler = handler;
		nextLabel->Data = 0;
		nextLabel->Position = 0;
		nextLabel[1].Previous = static_cast<std::uint16_t>(id);

		id = nextId;
		label = nextLabel;
		s = nextSegment;
	}
	label[1].Position = static_cast<std::uint16_t>(end.Position);
	return { retSegment, retId };
}

void Mimi::TextDocument::DeleteLabel(DocumentLabelIndex label)
{
	TextSegment* s = label.Segment;
	std::size_t index = label.Index;
	LabelData* d = s->ReadLabelData(index);
	assert((d->Type & LabelType::Continuous) == 0);

	if (d->Type & LabelType::Referred)
	{
		DocumentPositionD d = SegmentTree.ConvertPositionToD({ s, 0 });
		s->NotifyLabelRemoved(label.Index, d.Position);
	}

	while (d->Type & LabelType::Unfinished)
	{
		assert((d->Type & LabelType::Topology) == LabelType::Range);
		TextSegment* nextSegment = s->GetNextSegment();
		assert(nextSegment);
		std::size_t nextIndex = d[1].Next;
		LabelData* nextLabel = nextSegment->ReadLabelData(nextIndex);
		assert(nextLabel[1].Previous == index);

		s->EraseLabelSpace(index, TextSegment::GetLabelLength(d));

		s = nextSegment;
		index = nextIndex;
		d = nextLabel;
	}

	s->EraseLabelSpace(index, TextSegment::GetLabelLength(d));
}

Mimi::DocumentPositionS Mimi::TextDocument::DeleteRange(std::uint32_t time,
	DocumentPositionS begin, DocumentPositionS end)
{
	assert(static_cast<TextDocument*>(begin.Segment->GetDocument()) == this);
	assert(static_cast<TextDocument*>(end.Segment->GetDocument()) == this);
	assert(begin.Segment == end.Segment ||
		TextSegment::ComparePosition(begin.Segment, end.Segment) < 0);
	assert(begin.Position <= begin.Segment->GetCurrentLength());
	assert(end.Position <= end.Segment->GetCurrentLength());
	assert(begin.Segment != end.Segment || begin.Position <= end.Position);

	if (begin.Segment == end.Segment && begin.Position == end.Position)
	{
		//Do nothing.
		return begin;
	}

	DocumentPositionD g = SegmentTree.ConvertPositionToD({ begin.Segment, 0 });

	//Check whether we should start from next segment of begin.
	if (begin.Position == begin.Segment->GetCurrentLength())
	{
		begin.Segment = begin.Segment->GetNextSegment();
		begin.Position = 0;
		assert(begin.Segment);
	}

	//Check whether we should also delete the segment of end.
	if (end.Position == end.Segment->GetCurrentLength())
	{
		end.Segment = end.Segment->GetNextSegment();
		end.Position = 0;
		assert(begin.Segment != end.Segment);
		//end.Segment can be nullptr here.
	}

	if (begin.Segment == end.Segment)
	{
		//Single segment
		begin.Segment->MarkModified(time);
		begin.Segment->ReplaceText(begin.Position, end.Position - begin.Position, nullptr, g.Position);
		return begin;
	}
	else
	{
		//Multiple segment.

		//First we need to process the begin segment, which, after deleting, has some
		//data before the start position.

		TextSegment* s;
		DocumentPositionS prev;
		TextSegment* checkNewline = nullptr;

		if (begin.Position == 0)
		{
			//Skip begin.Segment if all of its content is removed, because ReplaceText
			//cannot handle this situation.
			s = begin.Segment;
			//Set prev (return value) as the end of the previous segment.
			prev.Segment = s->GetPreviousSegment();
			//prev.Segment can be null, when deleting from the beginning.
			if (prev.Segment)
			{
				prev.Position = prev.Segment->GetCurrentLength();
			}
		}
		else
		{
			begin.Segment->MakeActive();
			begin.Segment->MarkModified(time);
			begin.Segment->ReplaceText(begin.Position,
				begin.Segment->GetCurrentLength() - begin.Position, nullptr, g.Position);

			g.Position += begin.Segment->GetCurrentLength();
			s = begin.Segment->GetNextSegment();

			//Set prev (return value) as begin.
			prev = begin;
			checkNewline = begin.Segment;
		}

		while (s != end.Segment)
		{
			std::size_t len = s->GetCurrentLength();
			TextSegment* next = s->GetNextSegment();

			//Remove labels.
			s->UpdateLabelsDeleteAll(prev.Segment, end.Segment, g.Position);

			//Remove from list.
			TextSegment* checkRemoved = s->GetParent()->RemoveElement(s->GetIndexInList());
			assert(checkRemoved == s);

			//Release memory.
			delete s;

			g.Position += len;
			s = next;
			
			assert(!end.Segment || s);
		}
		if (end.Segment)
		{
			end.Segment->MakeActive();
			end.Segment->MarkModified(time);
			end.Segment->ReplaceText(0, end.Position, nullptr, g.Position);
		}
		if (checkNewline)
		{
			checkNewline->CheckLineBreak(); //This will also update end.Continuous if necessary.
		}
		if (prev.Segment == nullptr)
		{
			//Return the beginning position.
			return { SegmentTree.GetFirstSegment(), 0 };
		}
		return prev;
	}
}

static bool CheckInsertBuffer(Mimi::CodePage cp, Mimi::DynamicBuffer& content, bool hasNewline)
{
	using namespace Mimi;
	const mchar8_t* ptr = content.GetRawData();
	const mchar8_t* end = ptr + content.GetLength();
	std::size_t nNewLine = 0;

	char32_t ch;
	while (ptr < end)
	{
		ptr += cp.CharToUTF32(ptr, &ch);
		nNewLine += ch == '\n' ? 1 : 0;
	}

	if (hasNewline)
	{
		return ch == '\n' && nNewLine == 1;
	}
	return nNewLine == 0;
}

void Mimi::TextDocument::Insert(std::uint32_t time, DocumentPositionS pos, DynamicBuffer& content,
	bool hasNewline, DocumentPositionS* begin, DocumentPositionS* end)
{
	assert(CheckInsertBuffer(TextEncoding, content, hasNewline));

	//Don't insert after newline character.
	assert(pos.Segment->IsUnfinished() || //Unfinished segment
		(pos.Segment->GetCurrentLength() == 0 && pos.Segment->GetNextSegment() == nullptr) || //Last segment
		pos.Position < pos.Segment->GetCurrentLength()); //Normal segment: must before newline

	DocumentPositionD d = SegmentTree.ConvertPositionToD({ pos.Segment, 0 });

	if (hasNewline)
	{
		pos.Segment->MakeActive();
		pos.Segment->MarkModified(time);
		DocumentPositionS nextPos = pos.Segment->InsertLineBreak(pos.Position);
		DocumentPositionS newPos;

		//When using EnsureInsertionSize, we must convert the position accordingly.
		d.Position += pos.Position;
		pos.Segment->EnsureInsertionSize(pos.Position, content.GetLength(), &newPos);
		d.Position -= newPos.Position;

		newPos.Segment->ReplaceText(newPos.Position, 0, &content, d.Position);
		if (begin)
		{
			*begin = newPos;
		}
		if (end)
		{
			*end = nextPos;
		}
	}
	else
	{
		pos.Segment->MakeActive();
		pos.Segment->MarkModified(time);
		DocumentPositionS newPos1, newPos2;

		//When using EnsureInsertionSize, we must convert the position accordingly.
		d.Position += pos.Position;
		pos.Segment->EnsureInsertionSize(pos.Position, content.GetLength(), &newPos1, &newPos2);
		d.Position -= newPos1.Position;

		newPos1.Segment->MarkModified(time);
		newPos1.Segment->ReplaceText(newPos1.Position, 0, &content, d.Position);
		if (begin)
		{
			*begin = newPos1;
		}
		if (end)
		{
			*end = newPos2;
		}
	}
}

Mimi::TextDocument * Mimi::TextDocument::CreateEmpty(CodePage cp)
{
	TextSegment* s = new TextSegment(false, false, ModifiedFlag::NotModified);
	s->ContentBuffer = StaticBuffer::CreateEmpty();

	TextDocument* doc = new TextDocument(s);
	doc->TextEncoding = cp;
	return doc;
}

Mimi::TextDocument* Mimi::TextDocument::CreateFromTextFile(FileTypeDetector* file)
{
	assert(file->ReadNextLine());
	assert(!file->IsCurrentLineContinuous());
	TextSegment* first = new TextSegment(file->CurrentLineData,
		false, file->IsCurrentLineUnfinished(), ModifiedFlag::NotModified);

	TextDocument* doc = new TextDocument(first);
	doc->TextEncoding = file->GetCodePage();

	while (file->ReadNextLine())
	{
		doc->SegmentTree.FastAppend(new TextSegment(file->CurrentLineData,
			file->IsCurrentLineUnfinished(),
			file->IsCurrentLineContinuous(), ModifiedFlag::NotModified));
	}
	doc->SegmentTree.UpdataAllCount();

	return doc;
}

bool Mimi::LabelOwnerChangedEvent::Update(DocumentLabelIndex* label)
{
	if (label->Segment != OldOwner)
	{
		return false;
	}
	std::size_t pos = label->Segment->ReadLabelData(label->Index)->Position;
	if (pos >= BeginPosition && pos < EndPosition)
	{
		label->Segment = NewOwner;
		label->Index += IndexChange;
		return true;
	}
	if (label->Index == SingleId)
	{
		label->Segment = NewOwner;
		label->Index += IndexChange;
		return true;
	}
	return false;
}
