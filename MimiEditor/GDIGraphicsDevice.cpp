#include "GDIGraphicsDevice.h"
#include <Windows.h>

using namespace Mimi;
using namespace Mimi::GDI;

Mimi::GraphicsDevice* const Mimi::GraphicsDevice::Instance = new GDIDevice();

inline Result<Bitmap*> Mimi::GDI::GDIDevice::CreateBitmap(const BitmapData* data)
{
	//::CreateDIBSection
	return ErrorCodes::NotImplemented;
}
