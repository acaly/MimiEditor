#pragma once

#include "CommonInternal.h"
#include <cstdint>
#include <type_traits>
#include <new>
#include <cstring>
#include <cassert>

namespace Mimi
{
	template <typename T>
	class ShortVector
	{
		static_assert(std::is_trivial<T>::value, "ShortVector only supports trivial types.");

	public:
		ShortVector()
		{
			Pointer = nullptr;
			Count = Capacity = 0;
		}

		ShortVector(ShortVector<T>&&) = delete;
		ShortVector(const ShortVector<T>&) = delete;

		~ShortVector()
		{
			delete[] Pointer;
			Pointer = nullptr;
			Count = Capacity = 0;
		}

	private:
		T* Pointer;
		std::uint16_t Count;
		std::uint16_t Capacity;

	public:
		void Append(T&& val)
		{
			EnsureExtra(1);
			new(&Pointer[Count++]) T(val);
		}

		void Insert(std::uint32_t pos, T&& val)
		{
			EnsureExtra(1);
			assert(pos <= Count && "ShortVector: insert after the end.");
			std::memmove(&Pointer[pos + 1], &Pointer[pos], (Count - pos) * sizeof(T));
			new(&Pointer[pos]) T(val);
			Count += 1;
		}

		void EnsureExtra(std::uint32_t num)
		{
			if (Count + num <= Capacity) return;
			if (Capacity == 0)
			{
				Capacity = 2;
				Pointer = new T[Capacity];
			}
			else
			{
				std::uint16_t newCapacity = Capacity * 2;
				while (newCapacity < Count + num)
				{
					newCapacity *= 2;
				}
				T* newPointer = new T[newCapacity];
				std::memcpy(newPointer, Pointer, Count * sizeof(T));
				delete[] Pointer;
				Pointer = newPointer;
				Capacity = newCapacity;
			}
		}

		//Ensure the Pointer is unchanged
		void RemoveRange(std::uint32_t start, std::uint32_t len)
		{
			std::memmove(&Pointer[start], &Pointer[start + len], (Count - start - len) * sizeof(T));
			Count -= len;
		}

		void RemoveAt(std::uint32_t pos)
		{
			RemoveRange(pos, 1);
		}

		T* GetPointer()
		{
			return Pointer;
		}

		void Clear()
		{
			Count = 0;
		}

		std::uint32_t GetCount()
		{
			return Count;
		}

		T operator [](std::uint32_t index)
		{
			return Pointer[index];
		}
	};
}
