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
		std::size_t Size;

	public:
		virtual ~WindowsFileReader()
		{
			::CloseHandle(hFile);
			hFile = INVALID_HANDLE_VALUE;
		}

	public:
		virtual std::size_t GetSize() override
		{
			assert(Size < SIZE_MAX);
			return static_cast<std::size_t>(Size);
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
			assert(li.QuadPart < SIZE_MAX);
			return static_cast<std::size_t>(newPtr.QuadPart);
		}

		virtual Result<> Read(std::uint8_t* buffer, std::size_t bufferLen, std::size_t* numRead) override
		{
			bool ret;
			if (bufferLen > MAXDWORD)
			{
				return ErrorCodes::InvalidArgument;
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
			//TODO GetLastError of ReadFile
			if (numRead)
			{
				*numRead = totalRead;
			}
			if (totalRead > 0)
			{
				return true;
			}
			else
			{
				return ErrorCodes::Unknown;
			}
		}

		virtual Result<> Skip(std::size_t num) override
		{
			assert(num <= Size);
			LARGE_INTEGER li;
			li.QuadPart = num;
			if (::SetFilePointerEx(hFile, li, 0, FILE_CURRENT))
			{
				return true;
			}
			else
			{
				return ErrorCodes::Unknown;
			}
		}

		virtual Result<> Reset() override
		{
			LARGE_INTEGER li;
			li.QuadPart = 0;
			if (::SetFilePointerEx(hFile, li, 0, FILE_BEGIN))
			{
				return true;
			}
			else
			{
				return ErrorCodes::Unknown;
			}
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

		virtual Result<IFileReader*> Read() override
		{
			HANDLE h = ::CreateFile(reinterpret_cast<LPCWSTR>(Path.Data),
				GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
			if (h == INVALID_HANDLE_VALUE)
			{
				DWORD e = ::GetLastError();
				if (e == ERROR_FILE_NOT_FOUND)
				{
					return ErrorCodes::FileNotFound;
				}
				return ErrorCodes::Unknown;
			}
			LARGE_INTEGER size;
			if (!::GetFileSizeEx(h, &size))
			{
				DWORD e = ::GetLastError();
				::CloseHandle(h);
				return ErrorCodes::Unknown;
			}
			if (size.QuadPart >= SIZE_MAX)
			{
				return ErrorCodes::FileTooLarge;
			}
			WindowsFileReader* reader = new WindowsFileReader();
			reader->hFile = h;
			reader->Size = size.QuadPart;
			return static_cast<IFileReader*>(reader);
		}

		virtual Result<IFileWriter*> Write(std::size_t pos) override
		{
			return ErrorCodes::NotImplemented;
		}

		virtual Result<IFileWatcher*> StartWatch() override
		{
			return ErrorCodes::NotImplemented;
		}
	};
}

Mimi::Result<Mimi::IFile*> Mimi::IFile::CreateFromPath(String path)
{
	return new WindowsFile(std::move(path));
}
