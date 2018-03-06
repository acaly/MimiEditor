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
		virtual Result<Bitmap*> CreateBuffer(std::size_t w, std::size_t h) = 0;
		virtual void DisposeBitmap(Bitmap* bitmap) = 0;

		virtual Result<Renderer*> GetSystemRenderer(SystemRendererId id) = 0;
		virtual void DisposeRenderer(Renderer*) = 0;

		virtual LineStyle CreateLineStyle(Color4F color, ScreenSize sz) = 0;
		virtual void DisposeLineStyle(LineStyle s) = 0;

		virtual FillStyle CreateFillStyle(Color4F color) = 0;
		virtual void DisposeFillStyle(FillStyle s) = 0;

		virtual Font* CreateFont(String fontface, ScreenSize sz) = 0;
		virtual void DisposeFont(Font* font) = 0;

	public:
		static GraphicsDevice* const Instance;
	};
}
