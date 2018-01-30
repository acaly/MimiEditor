#pragma once
#include <cstddef>
#include "Buffer.h"

namespace Mimi
{
	class Snapshot final
	{
	public:
		Snapshot(std::size_t historyIndex);
		Snapshot(const Snapshot&) = delete;
		Snapshot(Snapshot&&) = delete;
		Snapshot& operator= (const Snapshot&) = delete;
		~Snapshot();

	public:
		void AppendBuffer(StaticBuffer buffer);
		void ClearBuffer(); //Called by document on main thread.
	};
}
