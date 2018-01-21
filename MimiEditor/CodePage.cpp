#include "CodePage.h"
#include <locale>

static bool Initialized = false;
Mimi::CodePageManager Instance;

namespace
{
	using Mimi::mchar8_t;

	class UTF8Impl : public Mimi::CodePageImpl
	{
	private:
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
		virtual int NormalWidth()
		{
			return 1;
		}

		virtual Mimi::BufferIncrement CharToUTF16(const mchar8_t* src, char16_t* dest) override
		{
			if (src[0] < 0x80u)
			{
				dest[0] = src[0];
				return { 1, 1 };
			}
			else if (src[0] < 0xC0)
			{
				return { 0, 0 }; //error, not first UTF8 byte
			}
			else if (src[0] < 0xE0)
			{
				return WriteUTF16(2, (src[0] & 0x1F) << 6 | (src[1] & 0x3F), dest);
			}
			else if (src[0] < 0xF0)
			{
				return WriteUTF16(3,
					(src[0] & 0x0F) << 12 | (src[1] & 0x3F) << 6 | (src[2] & 0x3F), dest);
			}
			else if (src[0] < 0xF8)
			{
				return WriteUTF16(4,
					(src[0] & 0x07) << 18 | (src[1] & 0x3F) << 12 |
					(src[2] & 0x3F) << 6 | (src[3] & 0x3F), dest);
			}
			else
			{
				return { 0, 0 }; //error, not valid UTF byte
			}
		}

		virtual Mimi::BufferIncrement CharFromUTF16(const char16_t* src, mchar8_t* dest) override
		{
			char32_t unicode;
			std::uint8_t r = ReadUTF16(src, &unicode);

			if (unicode < 0x80u)
			{
				dest[0] = static_cast<mchar8_t>(unicode);
				return { r, 1 };
			}
			else if (unicode < 0x800u)
			{
				dest[0] = static_cast<mchar8_t>(0xC0u | (unicode >> 6));
				dest[1] = static_cast<mchar8_t>(0x80u | (unicode & 0x3Fu));
				return { r, 2 };
			}
			else if (unicode < 0x10000u)
			{
				dest[0] = static_cast<mchar8_t>(0xE0u | (unicode >> 12));
				dest[1] = static_cast<mchar8_t>(0x80u | ((unicode >> 6) & 0x3Fu));
				dest[2] = static_cast<mchar8_t>(0x80u | (unicode & 0x3Fu));
				return { r, 3 };
			}
			else if (unicode < 0x110000u)
			{
				dest[0] = static_cast<mchar8_t>(0xF0u | (unicode >> 18));
				dest[1] = static_cast<mchar8_t>(0x80u | ((unicode >> 12) & 0x3Fu));
				dest[2] = static_cast<mchar8_t>(0x80u | ((unicode >> 6) & 0x3Fu));
				dest[3] = static_cast<mchar8_t>(0x80u | (unicode & 0x3Fu));
				return { r, 4 };
			}
			return { 0, 0 };
		}
	};

	class UTF16Impl : public Mimi::CodePageImpl
	{
	public:
		virtual int NormalWidth()
		{
			return 2;
		}

		virtual Mimi::BufferIncrement CharToUTF16(const mchar8_t* src, char16_t* dest) override
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

		virtual Mimi::BufferIncrement CharFromUTF16(const char16_t* src, mchar8_t* dest) override
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

static void Set(Mimi::CodePageImpl* const& cp, Mimi::CodePageImpl* impl)
{
	const_cast<Mimi::CodePageImpl*>(cp) = impl;
}

Mimi::CodePageManager* Mimi::CodePageManager::GetInstance()
{
	if (!Initialized)
	{
		Set(Instance.UTF8.Impl, new UTF8Impl());
		Set(Instance.UTF16.Impl, new UTF16Impl());
	}
	return &Instance;
}
