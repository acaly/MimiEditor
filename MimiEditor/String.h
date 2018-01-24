#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <vector>
#include "CodePage.h"

namespace Mimi
{
	static_assert(sizeof(char16_t) == 2, "char16_t must be 16 bits.");
	static_assert(sizeof(char32_t) == 4, "char32_t must be 32 bits.");

	//String data with code page information. Used only in external API so
	//performance is not important.
	class String
	{
	public:
		String()
			: String(nullptr, 0, CodePageManager::GetInstance()->UTF8)
		{
		}

		String(const mchar8_t* data, std::size_t len, CodePage encoding)
			: Encoding(encoding)
		{
			std::size_t nullLen = encoding.NormalWidth();
			char16_t last = 0;
			BufferIncrement inc = encoding.CharToUTF16(&data[len - nullLen], &last);
			bool appendNull = inc.Source == 0 || last != 0;
			Length = len + (appendNull ? nullLen : 0);
			mchar8_t* newData = new mchar8_t[Length];
			std::memcpy(newData, data, len);
			if (appendNull)
			{
				std::memset(&newData[Length - nullLen], 0, nullLen);
			}
			Data = newData;
		}

		String(const String& s)
			: Encoding(s.Encoding)
		{
			CopyFrom(s);
		}

		String(String&& s)
			: Encoding(s.Encoding)
		{
			Data = s.Data;
			Length = s.Length;
			s.Data = nullptr;
			s.Length = 0;
		}

		String& operator= (const String& s)
		{
			CopyFrom(s);
		}

		~String()
		{
			if (Data)
			{
				delete[] Data;
				Data = nullptr;
			}
			Length = 0;
		}

	private:
		void CopyFrom(const String& s)
		{
			Length = s.Length;
			mchar8_t* newData = new mchar8_t[Length];
			std::memcpy(newData, s.Data, Length);
			Data = newData;
			Encoding = s.Encoding;
		}

	public:
		const mchar8_t* Data;
		std::size_t Length;
		CodePage Encoding;

	public:
		String ToCodePage(CodePage cp)
		{
			if (Encoding != CodePageManager::GetInstance()->UTF16)
			{
				return ToUtf16String().ToCodePage(cp);
			}
			std::vector<mchar8_t> buffer;
			mchar8_t conv[8]; //8 should be enough for any encoding
			const char16_t* src = reinterpret_cast<const char16_t*>(Data);
			const char16_t* srcEnd = reinterpret_cast<const char16_t*>(&Data[Length]);
			while (src < srcEnd)
			{
				BufferIncrement inc = cp.CharFromUTF16(src, conv);
				buffer.insert(buffer.end(), conv, conv + inc.Destination);
				src += inc.Source;
			}
			return String(buffer.data(), buffer.size(), cp);
		}

		String ToUtf16String()
		{
			std::vector<char16_t> buffer;
			char16_t conv[2];
			const mchar8_t* src = Data;
			const mchar8_t* srcEnd = &Data[Length];
			while (src < srcEnd)
			{
				BufferIncrement inc = Encoding.CharToUTF16(src, conv);
				buffer.insert(buffer.end(), conv, conv + inc.Destination);
				src += inc.Source;
			}
			return String(reinterpret_cast<mchar8_t*>(buffer.data()),
				buffer.size() * 2, CodePageManager::GetInstance()->UTF16);
		}

		String ToUtf8String()
		{
			return ToCodePage(CodePageManager::GetInstance()->UTF8);
		}

	public:
		template <std::size_t N>
		static String FromUtf8(const char (&data)[N])
		{
			return String(reinterpret_cast<const mchar8_t*>(data), N, CodePageManager::GetInstance()->UTF8);
		}

		template <std::size_t N>
		static String FromUtf16(const char16_t (&data)[N])
		{
			return String(reinterpret_cast<const mchar8_t*>(data), N * 2, CodePageManager::GetInstance()->UTF16);
		}

		static String FromUtf8Ptr(const char* data)
		{
			return String(reinterpret_cast<const mchar8_t*>(data), std::strlen(data),
				CodePageManager::GetInstance()->UTF8);
		}
	};
}
