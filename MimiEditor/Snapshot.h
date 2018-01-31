#pragma once
#include <cstddef>
#include <cassert>
#include <vector>
#include "Buffer.h"

namespace Mimi
{
	class TextDocument;
	class SnapshotPositionConverter;

	//TODO use AbstractDocument instead of TextDocument.
	class Snapshot final
	{
		friend class TextDocument;

	public:
		Snapshot(TextDocument* doc, std::size_t historyIndex)
			: Document(doc), HistoryIndex(historyIndex)
		{
		}

		Snapshot(const Snapshot&) = delete;
		Snapshot(Snapshot&&) = delete;
		Snapshot& operator= (const Snapshot&) = delete;

		~Snapshot()
		{
			assert(BufferList.size() == 0);
		}

	private:
		TextDocument* Document;
		std::size_t HistoryIndex;
		std::vector<StaticBuffer> BufferList;

	private:
		void AppendBuffer(StaticBuffer buffer)
		{
			BufferList.push_back(buffer.NewRef());
		}

		void ClearBuffer()
		{
			for (auto&& buffer : BufferList)
			{
				buffer.ClearRef();
			}
			BufferList.clear();
		}

	public:
		TextDocument* GetDocument()
		{
			return Document;
		}

		std::size_t GetHistoryIndex()
		{
			return HistoryIndex;
		}

		SnapshotPositionConverter* CreatePositionConverter();
	};
}
