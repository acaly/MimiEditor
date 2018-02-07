#include "FileTypeDetector.h"
#include <unordered_set>
#include <algorithm>

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
			TotalCount = PositiveCount = NegativeCount = 0;
		}

	private:
		void CountUTF16BOChar(char32_t ch)
		{
			if (Type != BasicType::UTF16BE && Type != BasicType::UTF16LE)
			{
				return;
			}
			TotalCount += 1;
			if (ch <= 0x7F)
			{
				PositiveCount += 1;
			}
			if (ch < 0x10000 && (ch & 0xFF) == 0)
			{
				NegativeCount += 1;
			}
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
			CountUTF16BOChar(unicode);
			return true;
		}

		//No new character appended to ch.
		bool TestLast(const mchar8_t* ch, std::unordered_set<char32_t>& invalidUnicode)
		{
			while (BufferLength)
			{
				char32_t unicode;
				std::uint8_t r = Encoding.CharToUTF32(ch - BufferLength + 1, &unicode);
				if (r == 0)
				{
					//Failed or read exceed buffer range.
					//Read is still safe because buffer is longer (see caller).
					return false;
				}
				BufferLength -= (BufferLength < r) ? BufferLength : r;
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
				CountUTF16BOChar(unicode);
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

		float GetBOFactor()
		{
			if (TotalCount == 0)
			{
				return 1.0f; //Other CodePages return 1.0f.
			}
			const float MaxVal = 100;
			if (NegativeCount == 0)
			{
				if (PositiveCount > TotalCount / 10 || PositiveCount > 10)
				{
					return MaxVal; //No negative, positive is not minor, return Max.
				}
				else
				{
					return 1.0f; //No negative, but positive is minor. return 1.0f.
				}
			}
			float ret = static_cast<float>(PositiveCount) / NegativeCount;
			if (ret > MaxVal) ret = MaxVal; //Otherwise return ratio.
			return ret;
		}

	private:
		CodePage Encoding;
		std::size_t BufferLength = 0;
		bool IsFirstChar = true;
		bool BOM = false;
		bool AllowBOM;
		BasicType Type;
		std::size_t TotalCount;
		std::size_t PositiveCount, NegativeCount;
	};
}

Mimi::FileTypeDetector::FileTypeDetector(std::unique_ptr<IFileReader> reader, FileTypeDetectionOptions options)
	: Reader(std::move(reader)), CurrentLineData(0)
{
	Options = options;
	Error = false;
	TextReadEOS = false;
	Detect();
}

bool Mimi::FileTypeDetector::ReadNextLine()
{
	assert(IsTextFile());
	if (TextReadEOS)
	{
		return false;
	}
	CurrentLineData.Clear();

	Continuous = Unfinished;
	Unfinished = false;

	std::size_t remain;
	bool readNotFinished = false;
	while (remain = Reader.GetRemaining())
	{
		mchar8_t buffer[4] = {}; //Ensure empty
		Reader.Peek(buffer, remain > 4 ? 4 : remain);
		char32_t unicode;
		std::uint8_t r = TextCodePage.CharToUTF32(buffer, &unicode);
		if (r == 0 || r > remain)
		{
			//Invalid character.
			Error = true;
			switch (Options.InvalidCharAction)
			{
			case InvalidCharAction::Cancel:
				return false;
			case InvalidCharAction::ReplaceQuestionMark:
				unicode = '?';
				r = 1;
				break;
			case InvalidCharAction::SkipByte:
				Reader.SkipPeeked(1);
				continue; //Continue while loop
			}
		}
		CurrentLineData.Append(buffer, r);
		Reader.SkipPeeked(r);
		//Line break: only check for '\n'
		if (unicode == '\n')
		{
			readNotFinished = true;
			break;
		}
		//Line too long.
		if (CurrentLineData.GetLength() >= MaxLineLength)
		{
			Unfinished = true;
			readNotFinished = true;
			break;
		}
	}
	//Instead of check after exiting the loop, we must set the flag before
	//exiting, beacuse even if the file ends with '\n', we must give an
	//empty line after it.
	TextReadEOS = !readNotFinished;
	return true;
}

void Mimi::FileTypeDetector::Detect()
{
	std::vector<CodePageCheck> check;
	check.emplace_back(CodePageManager::ASCII, false, BasicType::ASCII);
	check.emplace_back(CodePageManager::UTF8, true, BasicType::UTF8);
	for (auto cp : Options.PrimaryCodePages)
	{
		check.emplace_back(cp, false, BasicType::CodePage);
	}
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
	//For UTF-16 LE/BE checking. These values are not used in unicode.
	invalid.insert(0x0A00); 
	invalid.insert(0x0D00);
	invalid.insert(0x0A0D);

	//Buffer is 8 but during the loop only first 4 bytes are used.
	//Last 4 bytes are there to ensure CheckLast() only read within buffer.
	mchar8_t buffer[8] = {};
	std::size_t bytesRead = 0;

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
		bytesRead += 1;

		//Test
		for (std::size_t i = 0; i < check.size(); ++i)
		{
			if (!check[i].Test(&buffer[3], invalid))
			{
				check.erase(check.begin() + i);
				i -= 1;
			}
		}
	}
	//Read 4 more (if not EOF)
	std::size_t remain = Reader.GetRemaining();
	Reader.Read(&buffer[4], remain > 4 ? 4 : remain);
	for (std::size_t i = 0; i < check.size(); ++i)
	{
		if (!check[i].TestLast(&buffer[3], invalid))
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
		//One with BOM?
		std::size_t index = 0;
		std::size_t num = 0;
		for (std::size_t i = 0; i < check.size(); ++i)
		{
			if (check[i].HasBOM())
			{
				num += 1;
				index = i;
			}
		}
		if (num != 1)
		{
			//If not, compare BOFactor
			float val = 0;
			for (std::size_t i = 0; i < check.size(); ++i)
			{
				float newVal = check[i].GetBOFactor();
				if (newVal > val)
				{
					val = newVal;
					index = i;
				}
			}
		}

		BasicType b = check[index].GetType();
		bool hasBOM = check[index].HasBOM();
		CodePage e = check[index].GetEncoding();
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
			assert(!"Invalid BasicType");
			break;
		}
	}
}

void Mimi::FileTypeDetector::SetupBinary()
{
	TextFile = false;
	Content = Reader.Finish();
	Content->Reset();
}

void Mimi::FileTypeDetector::SetupText(TextFileEncoding e, CodePage cp, std::size_t skipBOM)
{
	TextFile = true;
	Reader.Reset();
	Reader.Skip(skipBOM);
	TextEncoding = e;
	TextCodePage = cp;
}

void Mimi::FileTypeDetector::SetupText(CodePage cp)
{
	TextFile = true;
	Reader.Reset();
	TextEncoding = TextFileEncoding::CodePage;
	TextCodePage = cp;
}
