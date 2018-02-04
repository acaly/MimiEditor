#pragma once
#include "TextDocumentPosition.h"
#include <cstdint>
#include <vector>

namespace Mimi
{
	class TextDocument;

	class TextDocumentLabelIterator final
	{
		struct LabelItem
		{
			std::size_t Index;
			std::size_t Position;
			bool operator < (const LabelItem& other)
			{
				return Position < other.Position;
			}
		};

	public:
		TextDocumentLabelIterator(std::uint16_t handler,
			int direction, bool rangeBegin, bool rangeEnd)
			: Handler(handler), Direction(direction), RangeBegin(rangeBegin), RangeEnd(rangeEnd)
		{
			Cached = false;
			CurrentPosition = { nullptr, 0 };
			CurrentLabel = { nullptr, 0 };
		}

		TextDocumentLabelIterator(const TextDocumentLabelIterator&) = delete;
		TextDocumentLabelIterator(TextDocumentLabelIterator&&) = delete;
		TextDocumentLabelIterator& operator= (const TextDocumentLabelIterator&) = delete;
		~TextDocumentLabelIterator() {}

	private:
		DocumentPositionS CurrentPosition;
		DocumentLabelIndex CurrentLabel;
		const std::uint16_t Handler;
		const std::int8_t Direction;
		bool Cached;
		const bool RangeBegin, RangeEnd;
		std::vector<LabelItem> CachedLabel;

	private:
		static DocumentLabelIndex GetLabelHead(DocumentLabelIndex l);

	public:
		DocumentPositionS GetCurrentPosition()
		{
			return CurrentPosition;
		}

		DocumentLabelIndex GetCurrentLabel()
		{
			return CurrentLabel;
		}

		bool GoNext()
		{
			if (CurrentPosition.Segment == nullptr)
			{
				return false;
			}
			if (!Cached)
			{
				MakeCache();
			}
			if (CachedLabel.size() == 0)
			{
				if (!MoveSegment()) return false;
			}
			LabelItem ret = CachedLabel.back();
			CachedLabel.pop_back();
			CurrentLabel = GetLabelHead({ CurrentPosition.Segment, ret.Index });
			CurrentPosition.Position = ret.Position;
			return true;
		}

		void SetPosition(DocumentPositionS pos)
		{
			CurrentPosition = pos;
			CurrentLabel = { nullptr, 0 };
			CachedLabel.clear();
			Cached = false;
		}

	private:
		bool MakeCache(TextSegment* s, std::size_t begin, std::size_t end);
		bool MakeCache();
		bool MoveSegment();

	public:
		//Be careful: this is designed for one-time use. Do not iterate. Not considering
		//Labels at same position, no cache.
		static bool DirectFind(DocumentPositionS pos, int direction,
			std::uint16_t handler, bool rangeBegin, bool rangeEnd, DocumentLabelIndex* ret);
	};
}
