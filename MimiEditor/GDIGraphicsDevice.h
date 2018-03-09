#pragma once
#include "GraphicsDevice.h"

namespace Mimi
{
	namespace GDI
	{
		class GDIDevice : public GraphicsDevice
		{
		public:
			virtual ~GDIDevice();

		public:
			virtual Result<Bitmap*> CreateBitmap(BitmapData* data) override;
			virtual Result<Bitmap*> CreateBuffer(Renderer* compatible,
				std::size_t w, std::size_t h) override;
			virtual Result<LineStyle*> CreateLineStyle(Color4I color, ScreenSize sz) override;
			virtual Result<FillStyle*> CreateFillStyle(Color4I color) override;
			virtual Result<Font*> CreateFont(String fontface, ScreenSize sz) override;
		};
	}
}
