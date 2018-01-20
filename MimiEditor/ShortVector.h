#pragma once
#include <cstdint>
#include <cstddef>
#include <type_traits>
#include <cstring>
#include <cassert>

namespace Mimi
{
	template <typename T>
	class ShortVector
	{
	public:
		static const std::size_t MaxCapacity = 0xFFFF;
		static_assert(std::is_trivial<T>::value, "ShortVector only supports trivial types.");

	public:
		ShortVector()
		{
			Pointer = nullptr;
			Count = Capacity = 0;
		}

		ShortVector(const ShortVector<T>&) = delete;
		ShortVector(ShortVector<T>&&) = delete;
		ShortVector<T>& operator= (const ShortVector<T>&) = delete;

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
			Pointer[Count++] = val;
		}

		void ApendRange(T* data, std::size_t num)
		{
			EnsureExtra(num);
			std::memcpy(&Pointer[Count], data, num * sizeof(T));
			Count += static_cast<std::uint16_t>(num);
		}

		void Insert(std::size_t pos, T&& val)
		{
			EnsureExtra(1);
			assert(pos <= Count && "ShortVector: insert after the end.");
			std::memmove(&Pointer[pos + 1], &Pointer[pos], (Count - pos) * sizeof(T));
			Pointer[pos] = val;
			Count += 1;
		}

		T* Emplace(std::size_t num = 1)
		{
			EnsureExtra(num);
			T* ret = &Pointer[Count];
			Count += static_cast<std::uint16_t>(num);
			return ret;
		}

		void EnsureExtra(std::size_t num)
		{
			assert(num < MaxCapacity - Count);
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
		void RemoveRange(std::size_t start, std::size_t len)
		{
			assert(start <= Count && len <= Count - start);
			std::memmove(&Pointer[start], &Pointer[start + len], (Count - start - len) * sizeof(T));
			Count -= static_cast<std::uint16_t>(len);
		}

		void RemoveAt(std::size_t pos)
		{
			RemoveRange(pos, 1);
		}

		void Clear()
		{
			Count = 0;
		}

		void Shink(std::size_t capacity)
		{
			if (capacity < Capacity && capacity >= Count)
			{
				assert(capacity > 0);
				T* newPointer = new T[capacity];
				std::memcpy(newPointer, Pointer, Count * sizeof(T));
				delete[] Pointer;
				Pointer = newPointer;
				Capacity = static_cast<std::uint16_t>(capacity);
			}
		}

		void Shink()
		{
			Shink(Count);
		}

		T* GetPointer()
		{
			return Pointer;
		}

		std::size_t GetCount()
		{
			return Count;
		}

		T operator [](std::size_t index)
		{
			return Pointer[index];
		}
	};
}
