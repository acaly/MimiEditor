#pragma once
#include <cstddef>
#include <vector>
#include <memory>
#include <cassert>
#include "CodePage.h"
#include "IFile.h"
#include "Buffer.h"

namespace Mimi
{
	struct FileTypeDetectionOptions
	{
		std::size_t MaxRead;
		std::size_t MinRead;
		std::vector<CodePage> AdditionalCodePages;
	};

	enum class TextFileEncoding
	{
		Unknown,
		UTF8,
		UTF16LE,
		UTF16BE,
		CodePage,
	};

	class FileTypeDetector final
	{
	public:
		FileTypeDetector(std::unique_ptr<IFileReader> reader, FileTypeDetectionOptions options);
		FileTypeDetector(const FileTypeDetector&) = delete;
		FileTypeDetector(FileTypeDetector&&) = delete;
		FileTypeDetector& operator= (const FileTypeDetector&) = delete;
		~FileTypeDetector();

	private:
		bool IsText;
		std::unique_ptr<IFileReader> TextContent;
		DynamicBuffer InternalBuffer;
		std::size_t LineIndex;
		std::size_t InternalBufferPointer;
		bool Continuous;
		bool Unfinished;
		bool EncodingDeceided;
		bool CodePageDecided;
		TextFileEncoding TextEncoding;
		CodePage TextCodePage;

	public:
		bool IsTextFile()
		{
			return IsText;
		}

		TextFileEncoding GetEncoding()
		{
			assert(IsTextFile());
			return TextEncoding;
		}

		CodePage GetCodePage()
		{
			assert(IsTextFile());
			return TextCodePage;
		}

	public:
		std::size_t GetCurrentLineIndex()
		{
			assert(IsTextFile());
			return LineIndex;
		}

		DynamicBuffer CurrentLineData;

		bool IsCurrentLineContinuous()
		{
			assert(IsTextFile());
			return Continuous;
		}

		bool IsCurrentLineUnfinished()
		{
			assert(IsTextFile());
			return Unfinished;
		}

		void ReadNextLine();
		
	public:
		const std::unique_ptr<IFileReader> BinaryContent;
	};
}
