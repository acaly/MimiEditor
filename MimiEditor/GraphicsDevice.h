#pragma once
#include "Result.h"
#include "RenderingStyles.h"
#include "PixelFormat.h"
#include "Metrics.h"
#include "String.h"
#include <cstddef>

namespace Mimi
{
	using SystemRendererId = void*;

	class Bitmap;
	class BitmapData;
	class Renderer;
	class Font;

	class GraphicsDevice
	{
	public:
		virtual Result<Bitmap*> CreateBitmap(const BitmapData* data) = 0;
		virtual Result<Bitmap*> CreateBuffer(Renderer* compatible, std::size_t w, std::size_t h) = 0;
		virtual Result<LineStyle*> CreateLineStyle(Color4F color, ScreenSize sz) = 0;
		virtual Result<FillStyle*> CreateFillStyle(Color4F color) = 0;
		virtual Result<Font*> CreateFont(String fontface, ScreenSize sz) = 0;

	public:
		static GraphicsDevice* const Instance;
	};
}
