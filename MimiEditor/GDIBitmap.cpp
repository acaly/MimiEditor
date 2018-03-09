#include "GDIBitmap.h"

using namespace Mimi;
using namespace Mimi::GDI;

Result<BitmapData*> Mimi::GDI::GDIDIBitmap::CopyData()
{
	return ErrorCodes::NotImplemented;
}

Result<Renderer*> Mimi::GDI::GDIDIBitmap::CreateRenderer()
{
	return ErrorCodes::NotImplemented;
}
