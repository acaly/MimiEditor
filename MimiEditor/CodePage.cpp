#include "CodePage.h"
#include "String.h"
#include <locale>

extern Mimi::String UTF8Name;
extern Mimi::String UTF16Name;

namespace
{
	using namespace Mimi;

	class UTF8Impl final : public CodePageImpl32
	{
	public:
		virtual String GetDisplayName() override
		{
			return UTF8Name;
		}

		virtual std::size_t GetNormalWidth() override
		{
			return 1;
		}

		virtual std::uint8_t CharToUTF32(const mchar8_t* src, char32_t* dest) override
		{
			if (src[0] < 0x80u)
			{
				*dest = src[0];
				return 1;
			}
			else if (src[0] < 0xC0)
			{
				return 0; //error, not first UTF8 byte
			}
			else if (src[0] < 0xE0)
			{
				*dest = (src[0] & 0x1F) << 6 | (src[1] & 0x3F);
				return 2;
			}
			else if (src[0] < 0xF0)
			{
				*dest = (src[0] & 0x0F) << 12 | (src[1] & 0x3F) << 6 | (src[2] & 0x3F);
				return 3;
			}
			else if (src[0] < 0xF8)
			{
				*dest = (src[0] & 0x07) << 18 | (src[1] & 0x3F) << 12 |
					(src[2] & 0x3F) << 6 | (src[3] & 0x3F);
				return 4;
			}
			else
			{
				return 0; //error, not valid UTF byte
			}
		}

		virtual std::uint8_t CharFromUTF32(char32_t unicode, mchar8_t* dest) override
		{
			if (unicode < 0x80u)
			{
				dest[0] = static_cast<mchar8_t>(unicode);
				return 1;
			}
			else if (unicode < 0x800u)
			{
				dest[0] = static_cast<mchar8_t>(0xC0u | (unicode >> 6));
				dest[1] = static_cast<mchar8_t>(0x80u | (unicode & 0x3Fu));
				return 2;
			}
			else if (unicode < 0x10000u)
			{
				dest[0] = static_cast<mchar8_t>(0xE0u | (unicode >> 12));
				dest[1] = static_cast<mchar8_t>(0x80u | ((unicode >> 6) & 0x3Fu));
				dest[2] = static_cast<mchar8_t>(0x80u | (unicode & 0x3Fu));
				return 3;
			}
			else if (unicode < 0x110000u)
			{
				dest[0] = static_cast<mchar8_t>(0xF0u | (unicode >> 18));
				dest[1] = static_cast<mchar8_t>(0x80u | ((unicode >> 12) & 0x3Fu));
				dest[2] = static_cast<mchar8_t>(0x80u | ((unicode >> 6) & 0x3Fu));
				dest[3] = static_cast<mchar8_t>(0x80u | (unicode & 0x3Fu));
				return 4;
			}
			return 0;
		}
	};

	class UTF16Impl final : public CodePageImpl16
	{
	public:
		virtual String GetDisplayName() override
		{
			return UTF16Name;
		}

		virtual std::size_t GetNormalWidth() override
		{
			return 2;
		}

		virtual BufferIncrement CharToUTF16(const mchar8_t* src, char16_t* dest) override
		{
			char16_t d0 = *reinterpret_cast<const char16_t*>(src);
			if (d0 >= 0xD800u && d0 < 0xE000u)
			{
				dest[0] = d0;
				dest[1] = *reinterpret_cast<const char16_t*>(&src[2]);
				return { 4, 2 };
			}
			else
			{
				dest[0] = d0;
				return { 2, 1 };
			}
		}

		virtual BufferIncrement CharFromUTF16(const char16_t* src, mchar8_t* dest) override
		{
			if (src[0] >= 0xD800u && src[0] < 0xE000u)
			{
				*reinterpret_cast<char16_t*>(dest) = src[0];
				*reinterpret_cast<char16_t*>(&dest[2]) = src[1];
				return { 2, 4 };
			}
			else
			{
				*reinterpret_cast<char16_t*>(dest) = src[0];
				return { 1, 2 };
			}
		}
	};
}

const Mimi::CodePage Mimi::CodePageManager::UTF8 = { new UTF8Impl() };
const Mimi::CodePage Mimi::CodePageManager::UTF16 = { new UTF16Impl() };

static Mimi::String UTF8Name = Mimi::String::FromUtf8("UTF-8");
static Mimi::String UTF16Name = Mimi::String::FromUtf8("UTF-16");

Mimi::String Mimi::CodePage::GetDisplayName() const
{
	return Impl->GetDisplayName();
}
