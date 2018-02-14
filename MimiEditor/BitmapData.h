#pragma once
#include <cstddef>
#include <cassert>
#include <vector>
#include "PixelFormat.h"

namespace Mimi
{
	class IFile;

	struct RawBitmapData
	{
		PixelFormat Format;
		std::size_t Width; //In pixels
		std::size_t Height;
		void* Pointer;
		std::size_t Stride; //In bytes

		PaletteData Palette;
	};

	class BitmapData final
	{
	public:
		BitmapData()
		{
			Width = Height = 0;
		}
		
		BitmapData(std::size_t w, std::size_t h)
		{
			Width = w;
			Height = h;
		}

		BitmapData(const BitmapData&) = delete;
		BitmapData(BitmapData&&) = delete;
		BitmapData& operator= (const BitmapData&) = delete;

		~BitmapData()
		{
			for (std::size_t i = 0; i < BitmapList.size(); ++i)
			{
				delete[] BitmapList[i].Pointer;
				delete[] BitmapList[i].Palette;
			}
			BitmapList.clear();
		}

	private:
		std::size_t Width, Height;
		std::vector<RawBitmapData> BitmapList;
		
	public:
		bool LoadFile(IFile* file)
		{
			LoadData(CreateRawDataFromFile(file));
		}

		bool LoadData(RawBitmapData data)
		{
			if (data.Width == 0 || data.Height == 0)
			{
				assert(data.Pointer == nullptr);
				assert(data.Palette == nullptr);
				//Data loading failed.
				//TODO error msg.
				return false;
			}
			assert(data.Pointer);
			assert(data.Stride >= data.Width * data.Format.GetPixelSize());
			if (Width == 0)
			{
				assert(Height == 0);
				Width = data.Width;
				Height = data.Height;
			}
			if (Width * data.Height != Height * data.Width)
			{
				//TODO error msg.
				return false;
			}
			for (auto&& i : BitmapList)
			{
				if (i.Width == data.Width)
				{
					//TODO error msg.
					return false;
				}
			}
			BitmapList.push_back(data);
		}

	public:
		RawBitmapData GetRawData(float ratio = 1.0f)
		{
			float fw = static_cast<float>(Width) * ratio;
			std::size_t w = static_cast<std::size_t>(fw);
			for (auto&& i : BitmapList)
			{
				if (i.Width == w)
				{
					return i;
				}
			}
			return { PixelFormat::Empty, 0, 0, 0, 0, 0 };
		}

	public:
		static RawBitmapData CreateRawDataFromFile(IFile* file);
	};
}
