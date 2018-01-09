#include "CodePages.h"
#include <locale>

static bool Initialized = false;
Mimi::CodePageManager Instance;

namespace
{
	class UTF8Impl : public Mimi::CodePageImpl
	{
	private:
		Mimi::BufferIncrement WriteUTF16(std::uint8_t r, std::uint32_t unicode, std::uint16_t* dest)
		{
			if (unicode < 0x10000)
			{
				dest[0] = unicode;
				return { r, 1 };
			}
			else
			{
				unicode -= 0x10000;
				dest[0] = 0xD800 + (unicode >> 10);
				dest[1] = 0xDC00 + (unicode & 0x3FF);
				return { r, 2 };
			}
		}

		int ReadUTF16(const std::uint16_t* src, std::uint32_t* unicode)
		{
			if (src[0] >= 0xD800 && src[0] < 0xE000)
			{
				if (src[0] >= 0xDC00)
				{
					return 0; //not lead surrogate
				}
				if (src[1] < 0xDC00 || src[1] >= 0xE000)
				{
					return 0; //not trail surrogate
				}
				*unicode = 0x10000 + ((src[0] - 0xD800) << 10 | (src[1] - 0xDC00));
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

		virtual Mimi::BufferIncrement CharToUTF16(const std::uint8_t* src, std::uint16_t* dest)
		{
			if (src[0] < 0x80)
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

		virtual Mimi::BufferIncrement CharFromUTF16(const std::uint16_t* src, std::uint8_t* dest)
		{
			std::uint32_t unicode;
			std::uint8_t r = ReadUTF16(src, &unicode);

			if (unicode < 0x80)
			{
				dest[0] = static_cast<std::uint8_t>(unicode);
				return { r, 1 };
			}
			else if (unicode < 0x800)
			{
				dest[0] = static_cast<std::uint8_t>(0xC0 | (unicode >> 6));
				dest[1] = static_cast<std::uint8_t>(0x80 | (unicode & 0x3F));
				return { r, 2 };
			}
			else if (unicode < 0x10000)
			{
				dest[0] = static_cast<std::uint8_t>(0xE0 | (unicode >> 12));
				dest[1] = static_cast<std::uint8_t>(0x80 | ((unicode >> 6) & 0x3F));
				dest[2] = static_cast<std::uint8_t>(0x80 | (unicode & 0x3F));
				return { r, 3 };
			}
			else if (unicode < 0x110000)
			{
				dest[0] = static_cast<std::uint8_t>(0xF0 | (unicode >> 18));
				dest[1] = static_cast<std::uint8_t>(0x80 | ((unicode >> 12) & 0x3F));
				dest[2] = static_cast<std::uint8_t>(0x80 | ((unicode >> 6) & 0x3F));
				dest[3] = static_cast<std::uint8_t>(0x80 | (unicode & 0x3F));
				return { r, 4 };
			}
			return { 0,0 };
		}
	};

	class UTF16Impl : public Mimi::CodePageImpl
	{
	public:
		virtual int NormalWidth()
		{
			return 2;
		}

		virtual Mimi::BufferIncrement CharToUTF16(const std::uint8_t* src, std::uint16_t* dest)
		{
			auto d0 = *reinterpret_cast<const std::uint16_t*>(src);
			if (d0 >= 0xD800 && d0 < 0xE000)
			{
				dest[0] = d0;
				dest[1] = *reinterpret_cast<const std::uint16_t*>(&src[2]);
				return { 4, 2 };
			}
			else
			{
				dest[0] = d0;
				return { 2, 1 };
			}
		}

		virtual Mimi::BufferIncrement CharFromUTF16(const std::uint16_t* src, std::uint8_t* dest)
		{
			if (src[0] >= 0xD800 && src[0] < 0xE000)
			{
				*reinterpret_cast<std::uint16_t*>(dest) = src[0];
				*reinterpret_cast<std::uint16_t*>(&dest[2]) = src[1];
				return { 2, 4 };
			}
			else
			{
				*reinterpret_cast<std::uint16_t*>(dest) = src[0];
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
