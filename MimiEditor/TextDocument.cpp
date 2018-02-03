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
