#pragma once
#include <cstdint>
#include <cstddef>

namespace Mimi
{
	struct Color4I
	{
		std::uint8_t A, R, G, B;

		Color4I() = default;

		Color4I(int r, int g, int b)
			: Color4I(255, r, g, b)
		{
		}

		Color4I(int a, int r, int g, int b)
		{
			A = static_cast<std::uint8_t>(a);
			R = static_cast<std::uint8_t>(r);
			G = static_cast<std::uint8_t>(g);
			B = static_cast<std::uint8_t>(b);
		}
	};

	typedef const Color4I* PaletteData;

	class PixelFormatImpl
	{
	public:
		virtual bool IsSupported() = 0;
		virtual std::size_t GetPixelBits() = 0;
		virtual bool FromBGRA(PaletteData palette, const Color4I* bgra, void* data, std::size_t numPixel) = 0;
		virtual bool ToBGRA(PaletteData palette, const void* data, Color4I* bgra, std::size_t numPixel) = 0;
	};

	struct PixelFormat
	{
	private:
		PixelFormatImpl* Impl;

	public:
		bool IsSupported()
		{
			return Impl->IsSupported();
		}

		std::size_t GetPixelBits()
		{
			return Impl->GetPixelBits();
		}

		bool FromBGRA(PaletteData palette, const Color4I* bgra, void* data, std::size_t numPixel)
		{
			return Impl->FromBGRA(palette, bgra, data, numPixel);
		}

		bool ToBGRA(PaletteData palette, const void* data, Color4I* bgra, std::size_t numPixel)
		{
			return Impl->ToBGRA(palette, data, bgra, numPixel);
		}

	public:
		static PixelFormat Empty; //No data
		static PixelFormat R8; //One channel (8 byte int)
		static PixelFormat BGRA8888; //4 channel (8 byte int)

		static PixelFormat Default; //One supported format by the platform (may be used in conversion).
	};
}
