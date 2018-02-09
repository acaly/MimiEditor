#pragma once

namespace Mimi
{
	class BitmapData;
	class Renderer;

	class Bitmap
	{
	public:
		virtual BitmapData* GetData() = 0;
		virtual Renderer* CreateRenderer() = 0;
	};
}
