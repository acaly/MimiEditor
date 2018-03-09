#pragma once

namespace Mimi
{
	class Renderer;

	class AbstractControl
	{
	public:
		virtual ~AbstractControl() {}

		virtual void OnCreated() = 0;
		virtual void OnDestroyed() = 0;

		virtual void OnPaint(Renderer* renderer) = 0;
	};
}
