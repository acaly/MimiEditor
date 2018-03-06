#pragma once

namespace Mimi
{
	struct PixelSize
	{
		float Value;

		PixelSize(float value) { Value = value; }
		operator float() { return Value; }
	};

	struct ScreenSize
	{
		float Value;

		ScreenSize(float value) { Value = value; }
		operator float() { return Value; }
	};

	template <typename T>
	struct Vector2
	{
		T X, Y;
	};

	template <typename T>
	struct Rect
	{
		T X1, Y1, X2, Y2;
	};

	using Vector2P = Vector2<PixelSize>;
	using Vector2S = Vector2<ScreenSize>;

	using RectP = Rect<PixelSize>;
	using RectS = Rect<ScreenSize>;

	template <typename T>
	struct Matrix3
	{
		T M[9];
	};

	using Matrix3P = Matrix3<PixelSize>;
	using Matrix3S = Matrix3<ScreenSize>;
}
