#pragma once
#include <cstdint>
#include <cstddef>
#include "String.h"
#include "EventHandler.h"

namespace Mimi
{
	class IFile;
	class IFileReader;
	class IFileWriter;
	class IFileWatcher;

	class IFile
	{
	public:
		virtual ~IFile() {}
		virtual String GetIdentifier() = 0;
		virtual bool IsReadonly() = 0;
		virtual IFileReader* Read() = 0;
		virtual IFileWriter* Write(std::size_t pos) = 0;
		virtual IFileWatcher* StartWatch() = 0;

	public:
		static IFile* CreateFromPath(String path);
	};

	class IFileReader
	{
	public:
		virtual ~IFileReader() {}
		virtual std::uint32_t GetSize() = 0;
		virtual bool Read(std::uint8_t* buffer, std::size_t bufferLen, std::size_t* numRead) = 0;
		virtual bool Skip(std::size_t num) = 0;
		virtual bool Reset() = 0;
		virtual std::size_t GetPosition() = 0;
	};

	class IFileWriter
	{
	public:
		virtual ~IFileWriter() {}
		virtual bool Write(std::uint8_t* data, std::size_t dataLen) = 0;
		virtual void Skip(std::size_t num) = 0;
		virtual void Reset() = 0;
		virtual std::size_t GetPosition() = 0;
	};

	struct FileModifiedEvent
	{
	};

	struct FileDeletedEvent
	{
	};

	class IFileWatcher
	{
	public:
		virtual ~IFileWatcher() {}
		Event<FileModifiedEvent, void> Modified;
		Event<FileDeletedEvent, void> Deleted;
	};
}
