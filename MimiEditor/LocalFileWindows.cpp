#include "File.h"
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
		std::uint64_t Size;

	public:
		virtual ~WindowsFileReader()
		{
			::CloseHandle(hFile);
			hFile = INVALID_HANDLE_VALUE;
		}

	public:
		virtual std::size_t GetSize() override
		{
			if (Size > SIZE_MAX) //TODO handle 0xFFFFFFFF on x86?
			{
				return SIZE_MAX;
			}
			return static_cast<std::size_t>(Size);
		}

		virtual bool Read(std::uint8_t* buffer, std::size_t bufferLen, std::size_t* numRead) override
		{
			bool ret;
			if (bufferLen > MAXDWORD)
			{
				return false;
			}
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
			LARGE_INTEGER li;
			li.QuadPart = num;
			return ::SetFilePointerEx(hFile, li, 0, FILE_CURRENT);
		}

		virtual bool Reset() override
		{
			LARGE_INTEGER li;
			li.QuadPart = 0;
			return ::SetFilePointerEx(hFile, li, 0, FILE_BEGIN);
		}

		virtual std::size_t GetPosition() override
		{
			LARGE_INTEGER li;
			li.QuadPart = 0;
			LARGE_INTEGER newPtr;
			if (!::SetFilePointerEx(hFile, li, &newPtr, FILE_CURRENT))
			{
				return SIZE_MAX;
			}
			if (newPtr.HighPart != 0)
			{
				return SIZE_MAX; //TODO handle 0xFFFFFFFF on x86?
			}
			return static_cast<std::size_t>(newPtr.QuadPart);
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
			WindowsFileReader* reader = new WindowsFileReader();
			reader->hFile = h;
			reader->Size = size.QuadPart;
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
