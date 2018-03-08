#include "GDIBitmap.h"

using namespace Mimi;
using namespace Mimi::GDI;

Result<BitmapData*> Mimi::GDI::GDIBitmap::CopyData()
{
	return ErrorCodes::NotImplemented;
}

Result<Renderer*> Mimi::GDI::GDIBitmap::CreateRenderer()
{
	return ErrorCodes::NotImplemented;
}
