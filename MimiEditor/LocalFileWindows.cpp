#include "IFile.h"
#include <utility>
#include <cassert>
#include <Windows.h>

//C++ file stream is not designed for binary nor for string (with encoding). Use Windows API instead.

namespace
{
	using namespace Mimi;

	class WindowsFileReader : public IFileReader
	{
	public:
		HANDLE hFile;
		DWORD Size;

	public:
		virtual ~WindowsFileReader()
		{
			::CloseHandle(hFile);
			hFile = INVALID_HANDLE_VALUE;
		}

	public:
		virtual std::uint32_t GetSize() override
		{
			return Size;
		}

		virtual bool Read(std::uint8_t* buffer, std::size_t bufferLen, std::size_t* numRead) override
		{
			bool ret;
			assert(bufferLen <= MAXDWORD);
			DWORD toRead = static_cast<DWORD>(bufferLen);
			DWORD totalRead = 0;
			DWORD numReadValue;
			do
			{
				ret = ::ReadFile(hFile, buffer, toRead, &numReadValue, 0);
				toRead -= numReadValue;
				totalRead += numReadValue;
			} while (ret && toRead > 0 && numReadValue > 0);
			if (numRead)
			{
				*numRead = totalRead;
			}
			return totalRead > 0;
		}

		virtual bool Skip(std::size_t num) override
		{
			assert(num <= Size);
			return ::SetFilePointer(hFile, static_cast<DWORD>(num),
				0, FILE_CURRENT) != INVALID_SET_FILE_POINTER;
		}

		virtual bool Reset() override
		{
			return ::SetFilePointer(hFile, 0, 0, FILE_BEGIN) != INVALID_SET_FILE_POINTER;
		}

		virtual std::size_t GetPosition() override
		{
			//File size is less than 0xFFFFFFFF
			return ::SetFilePointer(hFile, 0, 0, FILE_CURRENT);
		}
	};

	class WindowsFile : public IFile
	{
	public:
		WindowsFile(String path)
			: Path(path.ToUtf16String())
		{
		}

	public:
		String Path;

	public:
		virtual bool IsReadonly() override
		{
			return false;
		}

		virtual String GetIdentifier() override
		{
			return Path;
		}

		virtual IFileReader* Read() override
		{
			HANDLE h = ::CreateFile(reinterpret_cast<LPCWSTR>(Path.Data),
				GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
			if (h == INVALID_HANDLE_VALUE)
			{
				DWORD e = ::GetLastError();
				//TODO store or log the code?
				return nullptr;
			}
			LARGE_INTEGER size;
			if (!::GetFileSizeEx(h, &size))
			{
				DWORD e = ::GetLastError();
				//TODO store or log the code?
				::CloseHandle(h);
				return nullptr;
			}
			if (size.HighPart != 0 || size.LowPart == INVALID_FILE_SIZE)
			{
				//TODO store or log the code?
				::CloseHandle(h);
				return nullptr;
			}
			WindowsFileReader* reader = new WindowsFileReader();
			reader->hFile = h;
			reader->Size = size.LowPart;
			return reader;
		}

		virtual IFileWriter* Write(std::size_t pos) override
		{
			return nullptr;
		}

		virtual IFileWatcher* StartWatch() override
		{
			return nullptr;
		}
	};
}

Mimi::IFile* Mimi::IFile::CreateFromPath(String path)
{
	return new WindowsFile(std::move(path));
}
