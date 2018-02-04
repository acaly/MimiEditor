#pragma once
#include "TextDocumentPosition.h"
#include "TextDocumentLabel.h"
#include "TextSegment.h"
#include <cstdint>

namespace Mimi
{
	struct TextDocumentLabelAccess
	{
		TextDocumentLabelAccess(DocumentLabelIndex label)
			: Label (label)
		{
		}

	public:
		const DocumentLabelIndex Label;

	private:
		LabelData* GetLabelData()
		{
			return GetLabelData(Label);
		}

		static LabelData* GetLabelData(DocumentLabelIndex l)
		{
			return l.Segment->ReadLabelData(l.Index);
		}

		bool IsHeadLabel()
		{
			return (GetLabelData()->Type & LabelType::Continuous) == 0;
		}

	public:
		std::uint16_t GetHandlerId()
		{
			return GetLabelData()->Handler;
		}

		bool HasLongData()
		{
			assert(IsHeadLabel());
			return GetLabelData()->Type & LabelType::Long;
		}

		std::uint8_t& ShortData()
		{
			assert(IsHeadLabel());
			return GetLabelData()->Data;
		}

		std::uint32_t& LongData()
		{
			assert(IsHeadLabel());
			assert(HasLongData());
			return GetLabelData()[1].Additional;
		}

		DocumentPositionS GetBeginPosition()
		{
			assert(IsHeadLabel());
			return { Label.Segment, GetLabelData(Label)->Position };
		}

		DocumentPositionS GetEndPosition()
		{
			DocumentLabelIndex l = Label;
			while (GetLabelData(l)->Type & LabelType::Unfinished)
			{
				DocumentLabelIndex n = { l.Segment->GetNextSegment(), GetLabelData(l)[1].Next };
				assert(n.Segment);
				assert(GetLabelData(n)[1].Previous == l.Index);
				l = n;
			}
			return { l.Segment, GetLabelData(l)[1].Position };
		}
	};
}
