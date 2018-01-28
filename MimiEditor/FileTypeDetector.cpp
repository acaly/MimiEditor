#include "FileTypeDetector.h"
#include <unordered_set>

namespace
{
	using namespace Mimi;

	enum class BasicType : std::uint8_t
	{
		ASCII,
		UTF8,
		UTF16LE,
		UTF16BE,
		CodePage,
	};

	class CodePageCheck
	{
		static const std::size_t BufferSize = 4;

	public:
		CodePageCheck(CodePage cp, bool allowBOM, BasicType type)
		{
			Encoding = cp;
			AllowBOM = allowBOM;
			Type = type;
		}

	public:
		//ch: the last char in the 4-byte buffer
		bool Test(const mchar8_t* ch, std::unordered_set<char32_t>& invalidUnicode)
		{
			if (++BufferLength < 4)
			{
				//Not reaching maximum. Convert later.
				return true;
			}
			char32_t unicode;
			std::uint8_t r = Encoding.CharToUTF32(ch - BufferLength + 1, &unicode);
			if (r == 0)
			{
				//Enough char but failed. Stop here.
				return false;
			}
			BufferLength -= r;
			if (IsFirstChar)
			{
				if (unicode == 0xFEFF)
				{
					BOM = true;
					return AllowBOM;
				}
				IsFirstChar = false;
			}
			if (invalidUnicode.find(unicode) != invalidUnicode.end())
			{
				//Invalid unicode. Stop here.
				return false;
			}
			return true;
		}

		//No new character appended to ch.
		bool TestLast(const mchar8_t* ch, std::unordered_set<char32_t>& invalidUnicode)
		{
			while (BufferLength)
			{
				char32_t unicode;
				std::uint8_t r = Encoding.CharToUTF32(ch - BufferLength + 1, &unicode);
				if (r == 0 || r > BufferLength)
				{
					//Failed or read exceed buffer range.
					//Read is still safe because buffer is longer (see caller).
					return false;
				}
				BufferLength -= r;
				if (IsFirstChar)
				{
					if (unicode == 0xFEFF)
					{
						BOM = true;
						return AllowBOM;
					}
					IsFirstChar = false;
				}
				if (invalidUnicode.find(unicode) != invalidUnicode.end())
				{
					//Invalid unicode. Stop here.
					return false;
				}
			}
			return true;
		}

		bool HasBOM()
		{
			return BOM;
		}

		BasicType GetType()
		{
			return Type;
		}

		CodePage GetEncoding()
		{
			return Encoding;
		}

	private:
		CodePage Encoding;
		std::size_t BufferLength = 0;
		bool IsFirstChar = true;
		bool BOM = false;
		bool AllowBOM;
		BasicType Type;
	};
}

Mimi::FileTypeDetector::FileTypeDetector(std::unique_ptr<IFileReader> reader, FileTypeDetectionOptions options)
	: Reader(std::move(reader)), CurrentLineData(0)
{
	Options = options;
	Detect();
}

void Mimi::FileTypeDetector::Detect()
{
	std::vector<CodePageCheck> check;
	check.emplace_back(CodePageManager::ASCII, false, BasicType::ASCII);
	for (auto cp : Options.PrimaryCodePages)
	{
		check.emplace_back(cp, false, BasicType::CodePage);
	}
	check.emplace_back(CodePageManager::UTF8, true, BasicType::UTF8);
	check.emplace_back(CodePageManager::UTF16LE, true, BasicType::UTF16LE);
	check.emplace_back(CodePageManager::UTF16BE, true, BasicType::UTF16BE);
	for (auto cp : Options.AdditionalCodePages)
	{
		check.emplace_back(cp, false, BasicType::CodePage);
	}

	std::unordered_set<char32_t> invalid;
	invalid.insert(Options.InvalidUnicode.begin(), Options.InvalidUnicode.end());
	invalid.insert(0xFEFF); //BOM at the middle or in other code page is not allowed.
	invalid.insert(0); //null is not allowed.
	//TODO add some invalid ASCII char for LE/BE checking

	//Buffer is 8 but during the loop only first 4 bytes are used.
	//Last 4 bytes are there to ensure CheckLast() only read within buffer.
	mchar8_t buffer[8] = {};
	std::size_t bytesRead;

	while (!(
		check.size() == 1 && bytesRead >= Options.MinRead || //Stop when decided.
		bytesRead >= Options.MaxRead || //Stop when reaching maximum.
		check.size() == 0)) //Stop when no possible code page.
	{
		//Read
		if (!Reader.Check(1))
		{
			break;
		}
		std::memmove(&buffer[0], &buffer[1], 3);
		buffer[3] = Reader.Read<mchar8_t>();

		//Test
		for (std::size_t i = 0; i < check.size(); ++i)
		{
			if (check[i].Test(&buffer[3], invalid))
			{
				check.erase(check.begin() + i);
				i -= 1;
			}
		}
	}
	for (std::size_t i = 0; i < check.size(); ++i)
	{
		if (check[i].TestLast(&buffer[3], invalid))
		{
			check.erase(check.begin() + i);
			i -= 1;
		}
	}

	if (check.size() == 0)
	{
		SetupBinary();
	}
	else
	{
		BasicType b = check[0].GetType();
		bool hasBOM = check[0].HasBOM();
		CodePage e = check[0].GetEncoding();
		switch (b)
		{
		case BasicType::ASCII:
			assert(!hasBOM);
			SetupText(TextFileEncoding::ASCII, e, 0);
			break;
		case BasicType::UTF8:
			SetupText(hasBOM ? TextFileEncoding::UTF8_BOM : TextFileEncoding::UTF8,
				e, hasBOM ? 3 : 0);
			break;
		case BasicType::UTF16LE:
			SetupText(hasBOM ? TextFileEncoding::UTF16LE_BOM : TextFileEncoding::UTF16LE,
				e, hasBOM ? 2 : 0);
			break;
		case BasicType::UTF16BE:
			SetupText(hasBOM ? TextFileEncoding::UTF16BE_BOM : TextFileEncoding::UTF16BE,
				e, hasBOM ? 2 : 0);
			break;
		case BasicType::CodePage:
			assert(!hasBOM);
			SetupText(e);
			break;
		default:
			break;
		}
	}
}

void Mimi::FileTypeDetector::SetupBinary()
{
	IsText = false;
	Content->Reset();
}

void Mimi::FileTypeDetector::SetupText(TextFileEncoding e, CodePage cp, std::size_t skipBOM)
{
	IsText = true;
	Content->Reset();
	Content->Skip(skipBOM);
	LineIndex = -1;
	TextEncoding = e;
	TextCodePage = cp;
}

void Mimi::FileTypeDetector::SetupText(CodePage cp)
{
	IsText = true;
	Content->Reset();
	LineIndex = -1;
	TextEncoding = TextFileEncoding::CodePage;
	TextCodePage = cp;
}
