#pragma once
#include <cstdint>
#include <cstddef>
#include "CodePage.h"

namespace Mimi
{
	//String data with code page information. Used only in external API so
	//performance is not important.
	class String
	{
	public:
		String();
		String(const std::uint8_t* data, std::size_t len, CodePage encoding);
		String(const String& s);
		String(String&& s);
		String& operator= (const String& s);
		~String();

	public:
		const std::uint8_t* const Data;
		const std::size_t Length;
		const CodePage Encoding;

	public:
		String ToUtf8String();
		String ToUtf16String();

	public:
		template <std::size_t N>
		static String FromUtf8(char data[N])
		{
			return String();
		}

		template <std::size_t N>
		static String FromUtf16(char16_t data[N])
		{
			return String();
		}
	};
}
