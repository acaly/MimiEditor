#pragma once
#include <cstddef>
#include "PixelFormat.h"

namespace Mimi
{
	class IFile;

	struct RawBitmapData
	{
		PixelFormat Format;
		std::size_t Width;
		std::size_t Height;
		void* Pointer;
		std::size_t Stride;
	};

	class BitmapData final
	{
	private:
		BitmapData();
	public:
		BitmapData(const BitmapData&) = delete;
		BitmapData(BitmapData&&) = delete;
		BitmapData& operator= (const BitmapData&) = delete;
		~BitmapData();

	public:
		static BitmapData* Create();
		static BitmapData* Create(std::size_t w, std::size_t h);
		
	public:
		void LoadFile(IFile* file, float ratio = 0);
		void LoadData(RawBitmapData data, float ratio = 0);

	public:
		RawBitmapData GetRawData(float ratio = 1.0f);
	};
}
