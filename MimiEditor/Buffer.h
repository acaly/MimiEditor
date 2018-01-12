#pragma once
#include <cstdint>
#include <cstring>

namespace Mimi
{
	struct StaticBuffer
	{
	private:
		struct StaticBufferData
		{
			std::uint16_t RefCount;
			std::uint16_t Size;
			std::uint8_t RawData[1];
		};

		StaticBufferData* Data;

	public:
		void IncreaseRef()
		{
			Data->RefCount += 1;
		}

		void DecreaseRef()
		{
			if (--Data->RefCount == 0)
			{
				delete Data;
			}
		}

	public:
		std::uint16_t GetSize()
		{
			return Data->Size;
		}

		const std::uint8_t* GetRawData()
		{
			return Data->RawData;
		}
	};

	class DynamicBuffer
	{
	public:
		DynamicBuffer(std::uint32_t capacity)
		{
			Pointer = new uint8_t[capacity];
			Length = 0;
			Capacity = capacity;
			IsExternalBuffer = false;
			IsFastMode = false;
		}

		//Use an external memory block.
		DynamicBuffer(StaticBuffer buffer)
		{
			ExternalBuffer = buffer;
			buffer.IncreaseRef();
			Pointer = nullptr;
			Length = buffer.GetSize();
			Capacity = 0;
			IsExternalBuffer = true;
			IsFastMode = false;
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
				ExternalBuffer.DecreaseRef();
				Length = Capacity = 0;
			}
		}

	private:
		std::uint8_t* Pointer;
		StaticBuffer ExternalBuffer;
		std::uint16_t Length;
		std::uint16_t Capacity;
		bool IsExternalBuffer;
		bool IsFastMode;
		//TODO Fast mode storage

	public:
		std::uint32_t GetLength()
		{
			return Length;
		}

		std::uint32_t CopyTo(std::uint8_t* buffer, std::uint32_t bufferLen)
		{
			std::uint32_t copyLen = Length;
			if (bufferLen < copyLen)
			{
				copyLen = bufferLen;
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

	private:
		void EnsureInternalBuffer(std::uint32_t extra)
		{
			if (IsExternalBuffer)
			{
				std::uint16_t capacity = 16;
				while (capacity < Length + extra) capacity *= 2;
				std::uint8_t* newBuffer = new std::uint8_t[capacity];
				std::memcpy(newBuffer, ExternalBuffer.GetRawData(), Length);
				Pointer = newBuffer;
				Capacity = capacity;
				IsExternalBuffer = false;
				ExternalBuffer.DecreaseRef();
			}
			else if (Capacity < Length + extra)
			{
				std::uint16_t capacity = Capacity;
				while (capacity < Length + extra) capacity *= 2;
				std::uint8_t* newBuffer = new std::uint8_t[capacity];
				std::memcpy(newBuffer, Pointer, Length);
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

		void Insert(std::uint32_t pos, const std::uint8_t* data, std::uint32_t len)
		{
			Replace(pos, 0, data, len);
		}

		void Delete(std::uint32_t pos, std::uint32_t len)
		{
			Replace(pos, len, nullptr, 0);
		}

		void Replace(std::uint32_t pos, std::uint32_t sel, const std::uint8_t* data, std::uint32_t dataLen)
		{
			assert(pos + sel <= Length);
			assert(!IsFastMode); //TODO
			if (sel < dataLen)
			{
				EnsureInternalBuffer(dataLen - sel);
			}
			std::memmove(&Pointer[pos + dataLen], &Pointer[pos + sel], Length - pos - sel);
			if (dataLen) std::memcpy(&Pointer[pos], data, dataLen);
			Length -= sel;
			Length += dataLen;
		}

	public:
		void Shink()
		{
			if (IsExternalBuffer) return;
			std::uint16_t capacity = Capacity;
			while (capacity > Length + 4) capacity /= 2;
			std::uint8_t* newBuffer = new std::uint8_t[capacity];
			std::memcpy(newBuffer, Pointer, Length);
			delete[] Pointer;
			Pointer = newBuffer;
			Capacity = capacity;
		}

	public:
		void StartFastMode()
		{
			//TODO enable fast mode
		}

		void FinishFastMode(); //TODO
	};
}
