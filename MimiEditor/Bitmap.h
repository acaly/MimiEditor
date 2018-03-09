#pragma once
#include "Result.h"

namespace Mimi
{
	class BitmapData;
	class Renderer;

	class Bitmap
	{
	public:
		virtual ~Bitmap() {}
		virtual Result<BitmapData*> CopyData() = 0; //TODO really needed?
		virtual Result<Renderer*> CreateRenderer() = 0;

		//TODO get size?
	};
}
