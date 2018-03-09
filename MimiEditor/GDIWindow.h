#pragma once
#include "EventHandler.h"
#include "Renderer.h"
#include <Windows.h>

namespace Mimi
{
	class AbstractControl;

	namespace GDI
	{
		class GDIWindow
		{
		public:
			GDIWindow(AbstractControl* handler)
				: ControlHandler(handler)
			{
			}

			virtual ~GDIWindow() {} //TODO check window has been destroyed

		private:
			HWND HWnd;
			SIZE Size;
			AbstractControl* ControlHandler;

		public:
			SIZE GetSize()
			{
				return Size;
			}

		public:
			void CreateControlWindow(HWND parent, RECT position);

		protected:
			void OnDestroyed();
			void OnPaint(Renderer* renderer);

		private:
			static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

		private:
			static GDIWindow* GetGDIWindow(HWND hWnd);

		public:
			static void RegisterWindowClass();
			static void RunDefaultMessageLoop();
			static void RunTestWindow(GDIWindow* window);
		};
	}
}
