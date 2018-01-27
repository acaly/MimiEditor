#include "CodePage.h"
#include "String.h"
#include <Windows.h>
#include <unordered_set>
#include <unordered_map>

namespace
{
	using namespace Mimi;

	const UINT AllCodePageId[] =
	{
		932,
		936,
		950,
	};

	class WindowsCodePageImpl : public CodePageImpl
	{
	private:
		UINT CodePageId;
		String DisplayName;
		std::unordered_map<mchar8_t, char32_t> SingleByteChars;
		std::unordered_set<mchar8_t> MultiByteChars;

	private:
		//TODO move the 2 functions to separate file (shared with CodePage.cpp)
		Mimi::BufferIncrement WriteUTF16(std::uint8_t r, char32_t unicode, char16_t* dest)
		{
			if (unicode < 0x10000u)
			{
				dest[0] = static_cast<char16_t>(unicode);
				return { r, 1 };
			}
			else
			{
				unicode -= 0x10000u;
				dest[0] = static_cast<char16_t>(0xD800u + (unicode >> 10));
				dest[1] = static_cast<char16_t>(0xDC00u + (unicode & 0x3FF));
				return { r, 2 };
			}
		}

		std::uint8_t ReadUTF16(const char16_t* src, char32_t* unicode)
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

	public:
		virtual String GetDisplayName() override
		{
			return DisplayName;
		}

		virtual std::size_t GetNormalWidth() override
		{
			return 1; //TODO confirm this
		}

		virtual BufferIncrement CharToUTF16(const mchar8_t* src, char16_t* dest) override
		{
			//Try one byte
			mchar8_t c1 = *src;
			{
				auto findSingle = SingleByteChars.find(c1);
				if (findSingle != SingleByteChars.end())
				{
					return WriteUTF16(1, findSingle->second, dest);
				}
			}

			//Not one byte or not cached. Use MultiByteToWideChar.
			LPCCH srcCh = reinterpret_cast<LPCCH>(src);
			LPWSTR destW = reinterpret_cast<LPWSTR>(dest);

			int first = MultiByteChars.find(c1) == MultiByteChars.end() ? 1 : 2;
			for (std::uint8_t r = first; r < 4; ++r) //Assume maximum of 4
			{
				int conv = ::MultiByteToWideChar(CodePageId, MB_ERR_INVALID_CHARS, srcCh, r, destW, 2);
				if (conv)
				{
					if (r == 1)
					{
						char32_t unicode;
						if (ReadUTF16(dest, &unicode))
						{
							SingleByteChars[c1] = unicode;
						}
					}
					else
					{
						MultiByteChars.insert(c1);
					}
					return { r, static_cast<std::uint8_t>(conv) };
				}
				//Try with more
			}
			return { 0, 0 }; //Error
		}

		virtual BufferIncrement CharFromUTF16(const char16_t* src, mchar8_t* dest) override
		{
			char16_t c1 = *src;
			std::uint8_t r = (c1 >= 0xD800u && c1 < 0xE000u) ? 2 : 1;
			LPCWCH srcW = reinterpret_cast<LPCWCH>(src);
			LPSTR destCh = reinterpret_cast<LPSTR>(dest);
			BOOL failed;
			int conv = ::WideCharToMultiByte(CodePageId, 0, srcW, r, destCh, 4, 0, &failed);
			if (conv && !failed)
			{
				return { r, static_cast<std::uint8_t>(conv) };
			}
			return { 0, 0 };
		}

	public:
		static WindowsCodePageImpl* Create(UINT cp)
		{
			CPINFOEX info;
			if (!::GetCPInfoEx(cp, 0, &info))
			{
				return nullptr;
			}
			WindowsCodePageImpl* ret = new WindowsCodePageImpl();
			ret->CodePageId = cp;
			ret->DisplayName = String::FromUtf16Ptr(info.CodePageName);
			return ret;
		}
	};

	template <std::size_t N>
	std::vector<CodePage> CreateAllCodePages(const UINT (&list)[N])
	{
		std::vector<CodePage> ret;
		for (std::size_t i = 0; i < N; ++i)
		{
			WindowsCodePageImpl* impl = WindowsCodePageImpl::Create(list[i]);
			if (impl)
			{
				ret.emplace_back(impl);
			}
		}
		return std::move(ret);
	}
}

Mimi::CodePage Mimi::CodePageManager::GetSystemCodePage()
{
	static CodePage systemCodePage = { WindowsCodePageImpl::Create(::GetACP()) };
	return systemCodePage;
}

std::vector<Mimi::CodePage> Mimi::CodePageManager::ListCodePages()
{
	static std::vector<CodePage> AllCodePages = CreateAllCodePages(AllCodePageId);
	return AllCodePages;
}

Mimi::CodePage GetWindowsCodePage936()
{
	return { WindowsCodePageImpl::Create(936) };
}
