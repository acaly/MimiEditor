#pragma once
#include "TextDocumentPosition.h"
#include <cstddef>

namespace Mimi
{
	class Snapshot;
	class TextSegment;

	class SnapshotPositionConverter final
	{
		friend class Snapshot;

	public:
		SnapshotPositionConverter(Snapshot* s);
		SnapshotPositionConverter(const SnapshotPositionConverter&) = delete;
		SnapshotPositionConverter(SnapshotPositionConverter&&) = delete;
		SnapshotPositionConverter& operator= (const SnapshotPositionConverter&) = delete;
		~SnapshotPositionConverter() {}

	private:
		Snapshot* SnapshotPtr;
		std::size_t CurrentHistoryPosition;
		TextSegment* CurrentSegment;
		std::size_t CurrentOffsetInSegment;

	public:
		//Init refreshes the internal reference to the TextSegment, which can change (delete,
		//merge, etc.) if document is modified. So if possible change happens, before calling
		//Convert, the caller must call Init.
		void Init();
		DocumentPositionS Convert(std::size_t dataPos, int dir);

	private:
		void Seek(std::size_t dataPos);
	};
}
