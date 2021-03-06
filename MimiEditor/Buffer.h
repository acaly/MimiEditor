#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cassert>

namespace Mimi
{
	class DynamicBuffer;

	struct StaticBuffer
	{
		friend class DynamicBuffer;

	private:
		struct StaticBufferData
		{
			std::uint16_t RefCount;
			std::uint16_t Size;
			std::uint8_t RawData[1];
		};

		StaticBufferData* Data;

		static StaticBufferData* AllocateData(std::size_t size)
		{
			char* data = new char[sizeof(StaticBufferData) + size - 2];
			StaticBufferData* ptr = reinterpret_cast<StaticBufferData*>(data);
			return ptr;
		}

	private:
		void IncreaseRef()
		{
			assert(Data);
			Data->RefCount += 1;
		}

		void DecreaseRef()
		{
			assert(Data);
			if (--Data->RefCount == 0)
			{
				delete[] reinterpret_cast<char*>(Data);
			}
		}

	public:
		static StaticBuffer CreateEmpty()
		{
			StaticBufferData* data = new StaticBufferData();
			data->RefCount = 1;
			data->Size = 0;
			data->RawData[0] = 0;

			StaticBuffer ret;
			ret.Data = data;
			return ret;
		}

	public:
		StaticBuffer NewRef()
		{
			IncreaseRef();
			return *this;
		}

		StaticBuffer MoveRef()
		{
			StaticBuffer ret = *this;
			Clear();
			return ret;
		}

		void ClearRef()
		{
			assert(Data);
			if (--Data->RefCount == 0)
			{
				delete[] reinterpret_cast<char*>(Data);
			}
			Clear();
		}

		void TryClearRef()
		{
			if (Data)
			{
				ClearRef();
			}
		}

	public:
		std::uint16_t GetSize() const
		{
			assert(Data);
			return Data->Size;
		}

		const std::uint8_t* GetRawData() const
		{
			assert(Data);
			return Data->RawData;
		}

	public:
		void Clear()
		{
			Data = nullptr;
		}

		bool IsNull()
		{
			return Data != nullptr;
		}
	};

	class DynamicBuffer
	{
		static const std::size_t MaxCapacity = 0xFFFF;
		static const std::size_t MinCapacity = 32;

	public:
		DynamicBuffer(std::size_t capacity)
		{
			assert(capacity <= MaxCapacity);
			if (capacity < MinCapacity) capacity = MinCapacity;
			Pointer = new uint8_t[capacity];
			Length = 0;
			Capacity = static_cast<std::uint16_t>(capacity);
			IsExternalBuffer = false;
		}

		//Use an external memory block.
		DynamicBuffer(StaticBuffer buffer)
		{
			ExternalBuffer = buffer.NewRef();
			Pointer = nullptr;
			Length = buffer.GetSize();
			Capacity = 0;
			IsExternalBuffer = true;
		}

		DynamicBuffer(const DynamicBuffer&) = delete;
		DynamicBuffer(DynamicBuffer&&) = delete;
		DynamicBuffer& operator= (const DynamicBuffer&) = delete;

		~DynamicBuffer()
		{
			if (!IsExternalBuffer)
			{
				delete[] Pointer;
				Pointer = nullptr;
				Length = Capacity = 0;
			}
			else
			{
				IsExternalBuffer = false;
				ExternalBuffer.ClearRef();
				Length = Capacity = 0;
			}
		}

	private:
		std::uint8_t* Pointer;
		StaticBuffer ExternalBuffer;
		std::uint16_t Length;
		std::uint16_t Capacity;
		bool IsExternalBuffer;

	public:
		std::size_t GetLength() const
		{
			return Length;
		}

		//Readonly
		const std::uint8_t* GetRawData() const
		{
			if (IsExternalBuffer)
			{
				return ExternalBuffer.GetRawData();
			}
			else
			{
				return Pointer;
			}
		}

		std::uint8_t* GetPointer()
		{
			assert(!IsExternalBuffer);
			return Pointer;
		}

		std::size_t CopyTo(std::uint8_t* buffer, std::size_t pos, std::size_t len) const
		{
			std::size_t copyLen = Length - pos;
			if (len < copyLen)
			{
				copyLen = len;
			}
			if (IsExternalBuffer)
			{
				std::memcpy(buffer, ExternalBuffer.GetRawData(), copyLen);
			}
			else
			{
				std::memcpy(buffer, Pointer, copyLen);
			}
			return copyLen;
		}

		StaticBuffer MakeStaticBuffer() const
		{
			StaticBuffer::StaticBufferData* ptr = StaticBuffer::AllocateData(Length);
			ptr->RefCount = 1;
			ptr->Size = Length;
			if (IsExternalBuffer)
			{
				std::memcpy(ptr->RawData, ExternalBuffer.GetRawData(), Length);
			}
			else
			{
				std::memcpy(ptr->RawData, Pointer, Length);
			}

			StaticBuffer ret;
			ret.Data = ptr;
			return ret;
		}

	private:
		void EnsureInternalBuffer(std::size_t newSize, bool copy)
		{
			assert(newSize <= MaxCapacity);
			if (IsExternalBuffer)
			{
				std::uint16_t capacity = static_cast<std::uint16_t>(newSize);
				std::uint8_t* newBuffer = new std::uint8_t[capacity];
				if (copy)
				{
					std::size_t copyLen = Length < capacity ? Length : capacity;
					std::memcpy(newBuffer, ExternalBuffer.GetRawData(), copyLen);
				}
				Pointer = newBuffer;
				Capacity = capacity;
				IsExternalBuffer = false;
				ExternalBuffer.ClearRef();
			}
			else if (Capacity < newSize + 4) //Give 4 bytes more.
			{
				std::uint16_t capacity = Capacity + Capacity / 2;
				if (capacity < newSize) capacity = static_cast<std::uint16_t>(newSize);
				std::uint8_t* newBuffer = new std::uint8_t[capacity];
				if (copy)
				{
					std::size_t copyLen = Length < capacity ? Length : capacity;
					std::memcpy(newBuffer, Pointer, copyLen);
				}
				delete[] Pointer;
				Pointer = newBuffer;
				Capacity = capacity;
			}
		}

	public:
		void Clear()
		{
			Delete(0, Length);
		}

		void Append(const std::uint8_t* data, std::size_t len)
		{
			Replace(Length, 0, data, len);
		}

		void Insert(std::size_t pos, const std::uint8_t* data, std::size_t len)
		{
			Replace(pos, 0, data, len);
		}

		void Delete(std::size_t pos, std::size_t len)
		{
			Replace(pos, len, nullptr, 0);
		}

		void Replace(std::size_t pos, std::size_t sel, const std::uint8_t* data, std::size_t dataLen)
		{
			assert(pos + sel <= Length);
			assert(Length - sel + dataLen <= MaxCapacity);
			EnsureInternalBuffer(Length - sel + dataLen, true);
			std::memmove(&Pointer[pos + dataLen], &Pointer[pos + sel], Length - pos - sel);
			if (dataLen) std::memcpy(&Pointer[pos], data, dataLen);
			Length -= static_cast<std::uint16_t>(sel);
			Length += static_cast<std::uint16_t>(dataLen);
		}

		std::uint8_t* Append(std::size_t len)
		{
			return Replace(Length, 0, len);
		}

		std::uint8_t* Insert(std::size_t pos, std::size_t len)
		{
			return Replace(pos, 0, len);
		}

		std::uint8_t* Replace(std::size_t pos, std::size_t sel, std::size_t dataLen)
		{
			assert(pos + sel <= Length);
			assert(Length - sel + dataLen <= MaxCapacity);
			EnsureInternalBuffer(Length - sel + dataLen, true);
			std::memmove(&Pointer[pos + dataLen], &Pointer[pos + sel], Length - pos - sel);
			Length -= static_cast<std::uint16_t>(sel);
			Length += static_cast<std::uint16_t>(dataLen);
			return &Pointer[pos];
		}

		void FastAppend(const std::uint8_t* data, std::size_t len)
		{
			assert(!IsExternalBuffer);
			assert(Length + len <= Capacity);
			assert(len > 0);
			assert(len <= 4);
			*reinterpret_cast<std::uint32_t*>(&Pointer[Length]) =
				*reinterpret_cast<const std::uint32_t*>(data);
			Length += static_cast<std::uint16_t>(len);
		}

	public:
		void Shink()
		{
			if (IsExternalBuffer) return;
			std::uint16_t capacity = Capacity;
			while (capacity > Length + 4) capacity /= 2;
			if (capacity < MinCapacity) capacity = MinCapacity;
			std::uint8_t* newBuffer = new std::uint8_t[capacity];
			std::memcpy(newBuffer, Pointer, Length);
			delete[] Pointer;
			Pointer = newBuffer;
			Capacity = capacity;
		}

	public:
		void SplitLeft(DynamicBuffer& other, std::size_t pos)
		{
			other.EnsureInternalBuffer(pos, false);
			CopyTo(other.Pointer, 0, pos);
			Delete(0, pos);
			other.Length = static_cast<std::uint16_t>(pos);
		}

		void SplitRight(DynamicBuffer& other, std::size_t pos)
		{
			std::size_t move = Length - pos;
			other.EnsureInternalBuffer(move, false);
			CopyTo(other.Pointer, pos, move);
			Delete(pos, move);
			other.Length = static_cast<std::uint16_t>(move);
		}
	};
}
