#pragma once
#include <cstddef>
#include "Metrics.h"
#include "RenderingStyles.h"
#include "PixelFormat.h"

namespace Mimi
{
	typedef std::size_t StackCheck;

	class Font;
	class Bitmap;

	class Renderer
	{
	public:
		virtual ~Renderer() {}

		virtual float GetResolution() = 0;

		virtual void Clear(Color4I color) = 0;

		virtual StackCheck PushClip(RectS clip) = 0;
		virtual void PopClip(StackCheck id) = 0;

		virtual StackCheck PushTransform(const Matrix3S& clip) = 0;
		virtual void PopTransform(StackCheck id) = 0;

		virtual void DrawLine(LineStyle* s, Vector2S p1, Vector2S p2) = 0;
		virtual void FillRect(FillStyle* s, RectS rect) = 0;

		virtual void DrawCircle(LineStyle* s, Vector2S o, ScreenSize r, float degFrom, float degTo) = 0;
		virtual void FillCircle(FillStyle* s, Vector2S o, ScreenSize r, float degFrom, float degTo) = 0;

		virtual void DrawImage(Bitmap* bitmap, const RectP& src, const RectS& dest) = 0;

		virtual void DrawCharacter(Font* font, Vector2S p, char32_t ch) = 0;
	};
}
