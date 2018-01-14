#pragma once
#include <cstdint>

namespace Mimi
{
	struct BufferIncrement
	{
		std::uint8_t Source;
		std::uint8_t Destination;
	};

	class CodePageImpl
	{
	public:
		virtual int NormalWidth() = 0;

		//Convert a character from the code page to utf16.
		//src: char data. This function may read a maximum of 4 bytes from here.
		//dest: buffer. This function may write the first 2 or 4 bytes to here.
		virtual BufferIncrement CharToUTF16(const std::uint8_t* src, std::uint16_t* dest) = 0;
		
		//Convert a character from utf16 to the code page.
		//src: utf16 char data. This function may read the first 2 or 4 bytes from here.
		//dest: char data. This function may write a maximum of 4 bytes to here.
		virtual BufferIncrement CharFromUTF16(const std::uint16_t* src, std::uint8_t* dest) = 0;
	};

	class CodePageManager;

	struct CodePage
	{
	private:
		CodePageImpl* Impl = nullptr;
		friend class CodePageManager;

	public:
		int NormalWidth() { return Impl->NormalWidth(); }

		BufferIncrement CharToUTF16(const std::uint8_t* src, std::uint16_t* dest)
		{
			return Impl->CharToUTF16(src, dest);
		}

		BufferIncrement CharFromUTF16(const std::uint16_t* src, std::uint8_t* dest)
		{
			return Impl->CharFromUTF16(src, dest);
		}
	};

	class CodePageManager
	{
	public:
		static CodePageManager* GetInstance();

	public:
		const CodePage UTF8;
		const CodePage UTF16;
	};
}
