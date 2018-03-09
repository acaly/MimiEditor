#include "GDIRenderer.h"

void Mimi::GDI::GDIRenderer::Clear(Color4I color)
{
	auto hBrush = ::CreateSolidBrush(RGB(color.R, color.G, color.B));
	::FillRect(HDC, &Region, hBrush);
	::DeleteObject(hBrush);
}
