#pragma once
#include <cstddef>
#include <cassert>
#include <vector>
#include "Buffer.h"

namespace Mimi
{
	class TextDocument;
	class SnapshotPositionConverter;
	class SnapshotReader;

	//TODO use AbstractDocument instead of TextDocument.
	class Snapshot final
	{
		friend class TextDocument;
		friend class SnapshotPositionConverter;
		friend class SnapshotReader;

	private:
		Snapshot(TextDocument* doc, std::size_t historyIndex)
			: Document(doc), HistoryIndex(historyIndex)
		{
		}

	public:
		Snapshot(const Snapshot&) = delete;
		Snapshot(Snapshot&&) = delete;
		Snapshot& operator= (const Snapshot&) = delete;

		~Snapshot();

	private:
		TextDocument* Document;
		std::size_t HistoryIndex;
		std::vector<StaticBuffer> BufferList;
		std::size_t DataLength;

	private:
		void AppendBuffer(StaticBuffer buffer)
		{
			BufferList.push_back(buffer.MoveRef());
		}

		void ClearBuffer()
		{
			for (auto&& buffer : BufferList)
			{
				buffer.ClearRef();
			}
			BufferList.clear();
		}

	private:
		TextDocument* GetDocument()
		{
			return Document;
		}

		std::size_t GetHistoryIndex()
		{
			return HistoryIndex;
		}
	};
}
