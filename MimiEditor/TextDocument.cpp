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
	do
	{
		s->AppendBuffer(segment->MakeSnapshot(resize));
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
	} while (segment);

	s->ClearBuffer();
}

Mimi::DocumentPositionS Mimi::TextDocument::DeleteRange(std::uint32_t time, DocumentPositionS begin, DocumentPositionS end)
{
	assert(static_cast<TextDocument*>(begin.Segment->GetDocument()) == this);
	assert(static_cast<TextDocument*>(end.Segment->GetDocument()) == this);
	assert(begin.Segment == end.Segment ||
		TextSegment::ComparePosition(begin.Segment, end.Segment) < 0);

	if (begin.Segment == end.Segment)
	{
		//Single segment
		begin.Segment->MarkModified(time);
		begin.Segment->ReplaceText(begin.Position, end.Position - begin.Position, nullptr);
		return begin;
	}
	else
	{
		//Multiple segment
		begin.Segment->MarkModified(time);
		begin.Segment->ReplaceText(begin.Position, begin.Segment->GetCurrentLength() - begin.Position, nullptr);
		TextSegment* s = begin.Segment->GetNextSegment();
		while (s != end.Segment)
		{
			TextSegment* removed = s->GetParent()->RemoveElement(s->GetIndexInList());
			s = s->GetNextSegment();
			assert(s);
			delete removed;
		}
		end.Segment->MarkModified(time);
		end.Segment->ReplaceText(0, end.Position, nullptr);
		begin.Segment->CheckLineBreak();
		end.Segment->CheckLineBreak();
		return begin;
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

	if (hasNewline)
	{
		pos.Segment->MarkModified(time);
		DocumentPositionS nextPos = pos.Segment->InsertLineBreak(pos.Position);
		DocumentPositionS newPos;
		pos.Segment->EnsureInsertionSize(pos.Position, content.GetLength(), &newPos);
		newPos.Segment->ReplaceText(newPos.Position, 0, &content);
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
		pos.Segment->MarkModified(time);
		DocumentPositionS newPos1, newPos2;
		pos.Segment->EnsureInsertionSize(pos.Position, content.GetLength(), &newPos1, &newPos2);
		newPos1.Segment->ReplaceText(newPos1.Position, 0, &content);
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

Mimi::TextDocument* Mimi::TextDocument::CreateFromTextFile(FileTypeDetector* file)
{
	assert(file->ReadNextLine());
	assert(!file->IsCurrentLineContinuous());
	TextSegment* first = new TextSegment(file->CurrentLineData,
		false, file->IsCurrentLineUnfinished(), ModifiedFlag::NotModified);

	TextDocument* doc = new TextDocument(first);

	while (file->ReadNextLine())
	{
		doc->SegmentTree.Append(new TextSegment(file->CurrentLineData,
			file->IsCurrentLineUnfinished(),
			file->IsCurrentLineContinuous(), ModifiedFlag::NotModified));
	}

	return doc;
}
