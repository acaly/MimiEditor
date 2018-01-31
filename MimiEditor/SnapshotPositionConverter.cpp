#include "SnapshotPositionConverter.h"
#include "Snapshot.h"
#include "TextDocument.h" //TODO change to abstract document class
#include "TextSegment.h"

Mimi::SnapshotPositionConverter::SnapshotPositionConverter(Snapshot * s)
	: SnapshotPtr(s)
{
	CurrentHistoryPosition = 0;
	CurrentOffsetInSegment = 0;
	CurrentSegment = SnapshotPtr->GetDocument()->SegmentTree.GetFirstSegment();
}

//TODO make it faster: can we avoid starting from the beginning?
void Mimi::SnapshotPositionConverter::Init()
{
	TextDocument* doc = SnapshotPtr->GetDocument();
	std::size_t sid = doc->ConvertSnapshotToIndex(SnapshotPtr->GetHistoryIndex());

	TextSegment* s = doc->SegmentTree.GetFirstSegment();
	std::size_t pos = CurrentHistoryPosition;
	std::size_t count = 0;
	std::size_t nextCount = s->GetHistoryLength(sid);
	while (nextCount <= pos)
	{
		count = nextCount;
		s = s->GetNextSegment();
		nextCount += s->GetHistoryLength(sid);
	}
	CurrentSegment = s;
	CurrentOffsetInSegment = pos - count;
}

Mimi::DocumentPositionS Mimi::SnapshotPositionConverter::Convert(std::size_t dataPos, int dir)
{
	TextDocument* doc = SnapshotPtr->GetDocument();
	std::size_t sid = doc->ConvertSnapshotToIndex(SnapshotPtr->GetHistoryIndex());

	Seek(dataPos);
	std::size_t currentPos = CurrentSegment->ConvertSnapshotPosition(sid, CurrentOffsetInSegment, dir);
	return { CurrentSegment, currentPos };
}

void Mimi::SnapshotPositionConverter::Seek(std::size_t dataPos)
{
	if (dataPos == CurrentHistoryPosition)
	{
		return;
	}

	TextDocument* doc = SnapshotPtr->GetDocument();
	std::size_t sid = doc->ConvertSnapshotToIndex(SnapshotPtr->GetHistoryIndex());

	TextSegment* s = CurrentSegment;

	if (dataPos < CurrentHistoryPosition)
	{
		//Backward
		std::size_t move = CurrentHistoryPosition - dataPos;

		if (CurrentOffsetInSegment > move)
		{
			//Within the same segment
			CurrentOffsetInSegment -= move;
			return;
		}

		//Scan backwards
		s = s->GetPreviousSegment();
		move -= CurrentOffsetInSegment;

		std::size_t hlen = s->GetHistoryLength(sid);
		while (move > hlen)
		{
			move -= hlen;
			s = s->GetPreviousSegment();
			hlen = s->GetHistoryLength(sid);
		}
		CurrentSegment = s;
		CurrentOffsetInSegment = hlen - move;
	}
	else
	{
		//Forward
		std::size_t move = dataPos - CurrentHistoryPosition;

		std::size_t hlen = s->GetHistoryLength(sid);
		if (move < hlen - CurrentOffsetInSegment)
		{
			//Within the same segment
			CurrentOffsetInSegment += move;
		}

		//Scan forward
		s = s->GetNextSegment();
		move -= hlen - CurrentOffsetInSegment;
		hlen = s->GetHistoryLength(sid);
		while (move > hlen)
		{
			move -= hlen;
			s = s->GetNextSegment();
			hlen = s->GetHistoryLength(sid);
		}
		CurrentSegment = s;
		CurrentOffsetInSegment = move;
	}
}
