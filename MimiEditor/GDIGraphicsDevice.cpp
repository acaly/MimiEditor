#include "GDIGraphicsDevice.h"
#include "GDIBitmap.h"
#include "BitmapData.h"
#include <vector>
#include <Windows.h>

using namespace Mimi;
using namespace Mimi::GDI;

Mimi::GraphicsDevice* const Mimi::GraphicsDevice::Instance = new GDIDevice();

static Result<HBITMAP> CreateGDIBitmapFromData(RawBitmapData* data)
{
	if (!data->Format.IsSupported())
	{
		//TODO raw data conversion
		return ErrorCodes::NotImplemented;
	}

	BITMAPINFO info;
	info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	info.bmiHeader.biWidth = static_cast<LONG>(data->Width);
	info.bmiHeader.biHeight = static_cast<LONG>(data->Height);
	info.bmiHeader.biPlanes = 1;
	info.bmiHeader.biBitCount = static_cast<WORD>(data->Format.GetPixelBits());
	info.bmiHeader.biCompression = BI_RGB; //Uncompressed.
	info.bmiHeader.biSize = 0; //Auto calculation for uncompressed.
	info.bmiHeader.biXPelsPerMeter = 96; //We select dpi ourselves.
	info.bmiHeader.biYPelsPerMeter = 96;
	info.bmiHeader.biClrUsed = 0; //No palette.
	info.bmiHeader.biClrImportant = 0;

	void* pData;
	HBITMAP ret = ::CreateDIBSection(NULL, &info, DIB_RGB_COLORS, &pData, NULL, 0);
	char* src = reinterpret_cast<char*>(data->Pointer);
	for (UINT i = 0; i < static_cast<UINT>(data->Height); ++i)
	{
		if (::SetDIBits(NULL, ret, i, 1, src, &info, DIB_RGB_COLORS) == 0)
		{
			::DeleteObject(ret);
			return ErrorCodes::Unknown;
		}
		src += data->Stride;
	}

	return ret;
}

Result<Bitmap*> Mimi::GDI::GDIDevice::CreateBitmap(BitmapData* data)
{
	std::vector<GDIDIBitmapData> list;
	std::size_t n = data->GetRawDataCount();
	RawBitmapData rawData;
	for (std::size_t i = 0; i < n; ++i)
	{
		rawData = data->GetRawData(i);
		Result<HBITMAP> b = CreateGDIBitmapFromData(&rawData);
		if (b.Success())
		{
			list.push_back({ b, rawData.Width });
		}
	}
	if (list.size() == 0)
	{
		return ErrorCodes::Unknown;
	}
	return new GDIDIBitmap(std::move(list));
}
