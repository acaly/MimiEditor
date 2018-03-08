#pragma once
#include "Bitmap.h"

namespace Mimi
{
	namespace GDI
	{
		class GDIBitmap : public Bitmap
		{
			virtual Result<BitmapData*> CopyData() override;
			virtual Result<Renderer*> CreateRenderer() override;
		};
	}
}
