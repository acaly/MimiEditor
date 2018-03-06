#pragma once
#include "Metrics.h"

namespace Mimi
{
	class Font
	{
	public:
		virtual ~Font() {}
		virtual RectS MeasureCharacter(char32_t ch) = 0;
		virtual ScreenSize GetKerning(char32_t left, char32_t right) = 0;
	};
}
