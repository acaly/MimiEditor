#pragma once
#include "Bitmap.h"
#include <vector>
#include <Windows.h>

namespace Mimi
{
	namespace GDI
	{
		struct GDIDIBitmapData
		{
			HBITMAP Handle;
			std::size_t Width;
		};

		class GDIDIBitmap : public Bitmap
		{
		public:
			GDIDIBitmap(std::vector<GDIDIBitmapData>&& list)
				: HandleList(list)
			{
			}
			~GDIDIBitmap();

		public:
			std::vector<GDIDIBitmapData> HandleList;

		public:
			virtual Result<BitmapData*> CopyData() override;
			virtual Result<Renderer*> CreateRenderer() override;
		};
	}
}
