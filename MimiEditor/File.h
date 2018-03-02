#pragma once
#include <cstdint>
#include <cstddef>
#include "String.h"
#include "EventHandler.h"
#include "Result.h"

namespace Mimi
{
	namespace ErrorCodes
	{
		DECLARE_ERROR_CODE_SIMPLE(FileNotFound);
		DECLARE_ERROR_CODE_SIMPLE(FileTooLarge);
		DECLARE_ERROR_CODE_SIMPLE(EndOfFile);
	}

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
		virtual Result<IFileReader*> Read() = 0;
		virtual Result<IFileWriter*> Write(std::size_t pos) = 0;
		virtual Result<IFileWatcher*> StartWatch() = 0;

	public:
		static Result<IFile*> CreateFromPath(String path);
	};

	class IFileReader
	{
	public:
		virtual ~IFileReader() {}
		virtual std::size_t GetSize() = 0;
		virtual std::size_t GetPosition() = 0;
		virtual Result<> Read(std::uint8_t* buffer, std::size_t bufferLen, std::size_t* numRead) = 0;
		virtual Result<> Skip(std::size_t num) = 0;
		virtual Result<> Reset() = 0;
	};

	class IFileWriter
	{
	public:
		virtual ~IFileWriter() {}
		virtual std::size_t GetPosition() = 0;
		virtual Result<> Write(std::uint8_t* data, std::size_t dataLen) = 0;
		virtual Result<> Skip(std::size_t num) = 0;
		virtual Result<> Reset() = 0;
		virtual Result<> Trim() = 0;
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
		//Error event?
	};
}
