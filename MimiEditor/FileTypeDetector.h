#pragma once
#include <cstddef>
#include <vector>
#include <memory>
#include <cassert>
#include "CodePage.h"
#include "IFile.h"
#include "Buffer.h"
#include "BinaryReader.h"

namespace Mimi
{
	enum class InvalidCharAction
	{
		SkipByte,
		ReplaceQuestionMark,
		Cancel,
	};

	struct FileTypeDetectionOptions
	{
		std::size_t MaxRead;
		std::size_t MinRead;
		std::vector<CodePage> PrimaryCodePages; //Checked before utf8 & utf16
		std::vector<CodePage> AdditionalCodePages; //Checked after utf8 & utf16
		std::vector<char32_t> InvalidUnicode;
		InvalidCharAction InvalidCharAction;
	};

	enum class TextFileEncoding
	{
		Unknown,
		ASCII,
		UTF8,
		UTF8_BOM,
		UTF16LE,
		UTF16BE,
		UTF16LE_BOM,
		UTF16BE_BOM,
		CodePage,
	};

	class FileTypeDetector final
	{
		static const std::size_t MaxLineLength = 256;

	public:
		FileTypeDetector(std::unique_ptr<IFileReader> reader, FileTypeDetectionOptions options);
		FileTypeDetector(const FileTypeDetector&) = delete;
		FileTypeDetector(FileTypeDetector&&) = delete;
		FileTypeDetector& operator= (const FileTypeDetector&) = delete;
		~FileTypeDetector() {}

	private:
		FileTypeDetectionOptions Options;
		BinaryReader Reader;

		std::unique_ptr<IFileReader> Content;
		bool TextFile;
		bool Continuous;
		bool Unfinished;
		bool Error;
		TextFileEncoding TextEncoding;
		CodePage TextCodePage;
	public:
		DynamicBuffer CurrentLineData;

	public:
		bool IsTextFile()
		{
			return TextFile;
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

		bool ReadNextLine();

		bool IsError()
		{
			return Error;
		}

		IFileReader* GetBinaryReader()
		{
			assert(!IsTextFile());
			return Content.get();
		}
		
	private:
		void Detect();
		void SetupBinary();
		void SetupText(TextFileEncoding e, CodePage cp, std::size_t skipBOM);
		void SetupText(CodePage cp);
	};
}
