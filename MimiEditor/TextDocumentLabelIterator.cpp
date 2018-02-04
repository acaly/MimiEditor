#include "TextDocumentLabelIterator.h"
#include "TextSegment.h"
#include <algorithm>

bool Mimi::TextDocumentLabelIterator::MakeCache()
{
	if (Direction > 0)
	{
		return MakeCache(CurrentPosition.Segment,
			CurrentPosition.Position, CurrentPosition.Segment->GetCurrentLength());
	}
	else
	{
		return MakeCache(CurrentPosition.Segment, 0, CurrentPosition.Position);
	}
}

static bool CheckRange(Mimi::LabelData* l, std::size_t begin, std::size_t end)
{
	return l->Position >= begin && l->Position < end;
}

Mimi::DocumentLabelIndex Mimi::TextDocumentLabelIterator::GetLabelHead(DocumentLabelIndex l)
{
	LabelData* labelData;
	while ((labelData = l.Segment->ReadLabelData(l.Index))->Type & LabelType::Continuous)
	{
		DocumentLabelIndex n = { l.Segment->GetPreviousSegment(), labelData[1].Previous };
		assert(n.Segment);
		assert(n.Segment->ReadLabelData(n.Index)[1].Next == l.Index);
		l = n;
	}
	return l;
}

bool Mimi::TextDocumentLabelIterator::MakeCache(TextSegment* s, std::size_t begin, std::size_t end)
{
	std::size_t l = s->FirstLabel();
	while (s->NextLabel(&l))
	{
		LabelData* d = s->ReadLabelData(l);
		//Check handler
		if (d->Handler != Handler) continue;
		//Check range and add
		if ((d->Type & LabelType::Topology) == LabelType::Range)
		{
			bool beginInRange = CheckRange(d, begin, end) && (d->Type & LabelType::Continuous) == 0;
			bool endInRange = CheckRange(d + 1, begin, end) && (d->Type & LabelType::Unfinished) == 0;
			if (RangeBegin && beginInRange)
			{
				CachedLabel.push_back({ l, d->Position });
			}
			if (RangeEnd && endInRange)
			{
				CachedLabel.push_back({ l, d[1].Position });
			}
		}
		else
		{
			if (!CheckRange(d, begin, end))
			{
				CachedLabel.push_back({ l, d->Position });
			}
		}
	}
	std::sort(CachedLabel.begin(), CachedLabel.end());
	//Because we always take from the end, reverse if forward.
	if (Direction > 0)
	{
		std::reverse(CachedLabel.begin(), CachedLabel.end());
	}
	Cached = true;
	return CachedLabel.size() > 0;
}

bool Mimi::TextDocumentLabelIterator::MoveSegment()
{
	TextSegment* s = CurrentPosition.Segment;
	s = Direction > 0 ? s->GetNextSegment() : s->GetPreviousSegment();
	while (s)
	{
		if (MakeCache(s, 0, s->GetCurrentLength()))
		{
			return true;
		}
		s = Direction > 0 ? s->GetNextSegment() : s->GetPreviousSegment();
	}
	return false;
}

bool Mimi::TextDocumentLabelIterator::DirectFind(DocumentPositionS pos, int direction,
	std::uint16_t handler, bool rangeBegin, bool rangeEnd, DocumentLabelIndex* ret)
{
	TextSegment* s = pos.Segment;
	std::size_t begin = 0;
	std::size_t end = s->GetCurrentLength();
	if (direction > 0)
	{
		begin = pos.Position;
	}
	else
	{
		end = pos.Position;
	}
	while (s)
	{
		std::size_t l = s->FirstLabel();
		bool found = false;
		//Use signed type for position for better comparison
		std::ptrdiff_t foundPos = direction > 0 ? end : begin;
		std::size_t foundIndex = 0;

		while (s->NextLabel(&l))
		{
			LabelData* d = s->ReadLabelData(l);
			//Check handler
			if (d->Handler != handler) continue;
			//Check range and add
			if ((d->Type & LabelType::Topology) == LabelType::Range)
			{
				bool beginInRange = CheckRange(d, begin, end) && (d->Type & LabelType::Continuous) == 0;
				bool endInRange = CheckRange(d + 1, begin, end) && (d->Type & LabelType::Unfinished) == 0;
				if (rangeBegin && beginInRange)
				{
					std::ptrdiff_t pos = d->Position;
					if (direction * (foundPos - pos) > 0)
					{
						found = true;
						foundPos = pos;
						foundIndex = l;
					}
				}
				if (rangeEnd && endInRange)
				{
					std::ptrdiff_t pos = d[1].Position;
					if (direction * (foundPos - pos) > 0)
					{
						found = true;
						foundPos = pos;
						foundIndex = l;
					}
				}
			}
			else
			{
				if (!CheckRange(d, begin, end))
				{
					std::ptrdiff_t pos = d->Position;
					if (direction * (foundPos - pos) > 0)
					{
						found = true;
						foundPos = pos;
						foundIndex = l;
					}
				}
			}
		}
		if (found)
		{
			*ret = GetLabelHead({ s, foundIndex });
			return true;
		}
		s = direction > 0 ? s->GetNextSegment() : s->GetPreviousSegment();
		begin = 0;
		end = s->GetCurrentLength();
	}
	return false;
}
