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
	};

	class CodePageManager
	{
	public:
		static CodePage GetSystemCodePage();
		static std::vector<CodePage> ListCodePages();

	public:
		static const CodePage UTF8;
		static const CodePage UTF16;
	};
}
