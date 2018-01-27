#pragma once
#include <memory>
#include <cassert>
#include "IFile.h"
#include "Buffer.h"

namespace Mimi
{
	class BinaryReader final
	{
		static const std::size_t BufferSize = 256;

	public:
		BinaryReader(std::unique_ptr<IFileReader> input)
			: Input(std::move(input)), Buffer(BufferSize),
			BufferPosition(0), InputMoved(false), InputFinished(false)
		{
			InputCount = Input->GetSize() - Input->GetPosition();
			Fill();
		}

		BinaryReader(const BinaryReader&) = delete;
		BinaryReader(BinaryReader&&) = delete;
		BinaryReader& operator= (const BinaryReader&) = delete;

		~BinaryReader()
		{
		}

	private:
		std::unique_ptr<IFileReader> Input;
		DynamicBuffer Buffer;
		std::size_t BufferPosition;
		std::size_t InputCount;
		bool InputFinished;
		bool InputMoved;

	private:
		bool CanRead()
		{
			return !InputMoved && !InputFinished;
		}

		void Fill()
		{
			assert(CanRead());
			std::size_t append = BufferSize - BufferPosition;
			Buffer.Delete(0, BufferPosition);
			BufferPosition = 0;
			std::uint8_t* space = Buffer.Append(append);
			std::size_t r;
			Input->Read(space, append, &r);
			InputCount -= r;
			InputFinished = r < append;
		}

		std::size_t BufferCount()
		{
			return Buffer.GetLength() - BufferPosition;
		}

	public:
		void Reset()
		{
			assert(!InputMoved);
			Input->Reset();
			Buffer.Clear();
			BufferPosition = 0;
			InputCount = Input->GetSize() - Input->GetPosition();
			InputFinished = false;
		}

		std::unique_ptr<IFileReader> Finish()
		{
			assert(!InputMoved);
			InputMoved = true;
			InputCount = 0;
			return std::move(Input);
		}

		bool Check(std::size_t size)
		{
			assert(size > 0);
			return InputCount >= size;
		}

		bool Read(void* buffer, std::size_t size)
		{
			assert(size < BufferSize);
			if (InputCount < size)
			{
				return false;
			}
			if (BufferCount() < size)
			{
				Fill();
			}
			std::memcpy(buffer, &Buffer.GetRawData()[BufferPosition], size);
			BufferPosition += size;
			return true;
		}

		template <typename T>
		bool Check()
		{
			return Check(sizeof(T));
		}

		template <typename T>
		T Read()
		{
			static_assert(std::is_trivial<T>::value, "Read non-trivial.");
			T ret;
			Read(&ret, sizeof(T));
			return ret;
		}

		template <typename T>
		bool Read(T* ret)
		{
			static_assert(std::is_trivial<T>::value, "Read non-trivial.");
			return Read(ret, sizeof(T));
		}
	};
}
