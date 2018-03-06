#pragma once

namespace Mimi
{
	class BitmapData;
	class Renderer;

	class Bitmap
	{
	public:
		virtual ~Bitmap() {}
		virtual BitmapData* CopyData() = 0;
		virtual Renderer* CreateRenderer() = 0;

		//TODO get size?
	};
}
