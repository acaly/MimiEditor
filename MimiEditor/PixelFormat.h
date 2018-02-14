#pragma once
#include <cstddef>

namespace Mimi
{
	struct Color4F
	{
		float A, R, G, B;
	};

	typedef  const Color4F* PaletteData;

	class PixelFormatImpl
	{
	public:
		virtual std::size_t GetPixelSize() = 0;
		virtual bool FromBGRA(PaletteData palette, const Color4F* bgra, void* data, std::size_t numPixel) = 0;
		virtual bool ToBGRA(PaletteData palette, const void* data, Color4F* bgra, std::size_t numPixel) = 0;
	};

	struct PixelFormat
	{
	private:
		PixelFormatImpl* Impl;
		std::size_t Id;
	public:
		//TODO
		std::size_t GetPixelSize()
		{
			return Impl->GetPixelSize();
		}

		bool FromBGRA(PaletteData palette, const Color4F* bgra, void* data, std::size_t numPixel)
		{
			return Impl->FromBGRA(palette, bgra, data, numPixel);
		}

		bool ToBGRA(PaletteData palette, const void* data, Color4F* bgra, std::size_t numPixel)
		{
			return Impl->ToBGRA(palette, data, bgra, numPixel);
		}
	public:
		//TODO more formats
		static PixelFormat Empty; //No data
	};
}
