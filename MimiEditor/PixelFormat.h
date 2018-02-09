#pragma once
#include <cstddef>

namespace Mimi
{
	class PixelFormatImpl
	{
	public:
		virtual std::size_t GetPixelSize() = 0;
		virtual void FromBGRA(const float* bgra, void* data, std::size_t numPixel) = 0;
		virtual void ToBGRA(const void* data, float* bgra, std::size_t numPixel) = 0;
	};

	struct PixelFormat
	{
	private:
		PixelFormatImpl* Impl;
		std::size_t Id;
	public:
		//TODO
	};
}
