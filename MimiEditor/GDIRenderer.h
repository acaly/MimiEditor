#pragma once
#include "Renderer.h"
#include <Windows.h>

namespace Mimi
{
	namespace GDI
	{
		class GDIRenderer : public Renderer
		{
		public:
			GDIRenderer(HWND hwnd, HDC hdc, RECT region)
				: HWnd(hwnd), HDC(hdc), Region(region)
			{
				Resolution = ::GetDpiForWindow(hwnd) / 96.0f;
			}

		private:
			HWND HWnd;
			HDC HDC;
			RECT Region;
			float Resolution;

		public:
			virtual float GetResolution() override
			{
				return Resolution;
			}

			virtual void Clear(Color4I color) override;

			virtual StackCheck PushClip(RectS clip) override { return 0; }
			virtual void PopClip(StackCheck id) override {}

			virtual StackCheck PushTransform(const Matrix3S & clip) override { return 0; }
			virtual void PopTransform(StackCheck id) override {}

			virtual void DrawLine(LineStyle* s, Vector2S p1, Vector2S p2) override {}
			virtual void FillRect(FillStyle* s, RectS rect) override {}
			virtual void DrawCircle(LineStyle* s, Vector2S o, ScreenSize r,
				float degFrom, float degTo) override {}
			virtual void FillCircle(FillStyle* s, Vector2S o, ScreenSize r,
				float degFrom, float degTo) override {}
			virtual void DrawImage(Bitmap* bitmap,
				const RectP & src, const RectS & dest) override {}
			virtual void DrawCharacter(Font* font, Vector2S p, char32_t ch) override {}

		private:
			Vector2P ConvertPoint(Vector2S point)
			{
				return { point.X * Resolution, point.Y * Resolution };
			}
		};
	}
}
