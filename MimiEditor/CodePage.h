#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace Mimi
{
	typedef std::uint8_t mchar8_t; //I hate C++

	class String;
	class CodePageManager;

	struct BufferIncrement
	{
		std::uint8_t Source;
		std::uint8_t Destination;
	};

	class UnicodeHelper
	{
		UnicodeHelper() {}

	public:
		static std::uint8_t ConvertUTF32To16(char32_t unicode, char16_t* dest)
		{
			if (unicode < 0x10000u)
			{
				dest[0] = static_cast<char16_t>(unicode);
				return 1;
			}
			else
			{
				unicode -= 0x10000u;
				dest[0] = static_cast<char16_t>(0xD800u + (unicode >> 10));
				dest[1] = static_cast<char16_t>(0xDC00u + (unicode & 0x3FF));
				return 2;
			}
		}

		static Mimi::BufferIncrement WriteUTF16(std::uint8_t r, char32_t unicode, char16_t* dest)
		{
			return { r, ConvertUTF32To16(unicode, dest) };
		}

		static std::uint8_t ConvertUTF16To32(const char16_t* src, char32_t* unicode)
		{
			if (src[0] >= 0xD800u && src[0] < 0xE000u)
			{
				if (src[0] >= 0xDC00u)
				{
					return 0; //not lead surrogate
				}
				if (src[1] < 0xDC00u || src[1] >= 0xE000u)
				{
					return 0; //not trail surrogate
				}
				*unicode = 0x10000u + ((src[0] - 0xD800u) << 10 | (src[1] - 0xDC00u));
				return 2;
			}
			else
			{
				*unicode = src[0];
				return 1;
			}
		}
	};

	class CodePageImpl
	{
	public:
		virtual String GetDisplayName() = 0;

		virtual std::size_t GetNormalWidth() = 0;

		//Convert a character from the code page to utf16.
		//src: char data. This function may read a maximum of 4 bytes from here.
		//dest: buffer. This function may write the first 2 or 4 bytes to here.
		virtual BufferIncrement CharToUTF16(const mchar8_t* src, char16_t* dest) = 0;
		
		//Convert a character from utf16 to the code page.
		//src: utf16 char data. This function may read the first 2 or 4 bytes from here.
		//dest: char data. This function may write a maximum of 4 bytes to here.
		virtual BufferIncrement CharFromUTF16(const char16_t* src, mchar8_t* dest) = 0;

		virtual std::uint8_t CharToUTF32(const mchar8_t* src, char32_t* dest) = 0;
		virtual std::uint8_t CharFromUTF32(char32_t unicode, mchar8_t* dest) = 0;
	};

	class CodePageImpl32 : public CodePageImpl
	{
	public:
		virtual BufferIncrement CharToUTF16(const mchar8_t* src, char16_t* dest) override final
		{
			char32_t unicode;
			if (std::uint8_t r = CharToUTF32(src, &unicode))
			{
				return UnicodeHelper::WriteUTF16(r, unicode, dest);
			}
			return { 0, 0 };
		}

		virtual BufferIncrement CharFromUTF16(const char16_t* src, mchar8_t* dest) override final
		{
			char32_t unicode;
			if (std::uint8_t r = UnicodeHelper::ConvertUTF16To32(src, &unicode))
			{
				if (std::uint8_t w = CharFromUTF32(unicode, dest))
				{
					return { r, w };
				}
			}
			return { 0, 0 };
		}
	};

	class CodePageImpl16 : public CodePageImpl
	{
	public:
		virtual std::uint8_t CharToUTF32(const mchar8_t* src, char32_t* dest) override final
		{
			char16_t utf16[2];
			std::uint8_t r = CharToUTF16(src, utf16).Source;
			UnicodeHelper::ConvertUTF16To32(utf16, dest);
			return r;
		}

		virtual std::uint8_t CharFromUTF32(char32_t unicode, mchar8_t* dest) override final
		{
			char16_t utf16[2];
			UnicodeHelper::ConvertUTF32To16(unicode, utf16);
			return CharFromUTF16(utf16, dest).Destination;
		}
	};

	struct CodePage
	{
		friend class CodePageManager;

	public:
		CodePage() = default;
		CodePage(CodePageImpl* impl) : Impl(impl) {}

	private:
		CodePageImpl* Impl = nullptr;

	public:
		bool IsValid() const
		{
			return Impl != nullptr;
		}

	public:
		bool operator== (const CodePage& cp) const
		{
			return Impl == cp.Impl;
		}

		bool operator!= (const CodePage& cp) const
		{
			return Impl != cp.Impl;
		}

	public:
		String GetDisplayName() const;

		std::size_t GetNormalWidth() const 
		{
			return Impl->GetNormalWidth();
		}

		BufferIncrement CharToUTF16(const mchar8_t* src, char16_t* dest) const
		{
			return Impl->CharToUTF16(src, dest);
		}

		BufferIncrement CharFromUTF16(const char16_t* src, mchar8_t* dest) const
		{
			return Impl->CharFromUTF16(src, dest);
		}

		std::uint8_t CharToUTF32(const mchar8_t* src, char32_t* dest) const
		{
			return Impl->CharToUTF32(src, dest);
		}

		std::uint8_t CharFromUTF32(char32_t unicode, mchar8_t* dest) const
		{
			return Impl->CharFromUTF32(unicode, dest);
		}
	};

	class CodePageManager
	{
	public:
		static CodePage GetSystemCodePage();
		static std::vector<CodePage> ListCodePages();

	public:
		static const CodePage UTF8;
		static const CodePage UTF16LE;
		static const CodePage UTF16BE;
		static const CodePage ASCII;
	};
}
