#pragma once
#include <cstdint>
#include <cstddef>
#include "String.h"

namespace Mimi
{
	class IFile;
	class IFileReader;
	class IFileWriter;

	class IFile
	{
	public:
		virtual ~IFile() {}
		virtual bool IsReadonly() = 0;
		virtual IFileReader* Read(std::size_t pos) = 0;
		virtual IFileWriter* Write(std::size_t pos) = 0;

	public:
		static IFile* CreateFromPath(String path);
	};

	class IFileReader
	{
	public:
		virtual ~IFileReader() {}
		virtual bool Read(std::uint8_t* buffer, std::size_t bufferLen, std::size_t* numRead) = 0;
	};

	class IFileWriter
	{
	public:
		virtual ~IFileWriter() {}
		virtual bool Write(std::uint8_t* data, std::size_t dataLen) = 0;
	};
}
