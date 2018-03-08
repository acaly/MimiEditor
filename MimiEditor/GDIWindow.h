#pragma once
#include "EventHandler.h"
#include <Windows.h>

namespace Mimi
{
	namespace GDI
	{
		class GDIWindow
		{
		public:
			GDIWindow() {}
			virtual ~GDIWindow() {} //TODO check window has been destroyed

		private:
			HWND HWnd;
			SIZE Size;

		public:
			HWND GetHWnd()
			{
				return HWnd;
			}

			SIZE GetSize()
			{
				return Size;
			}

		public:
			void CreateControlWindow(HWND parent, RECT position);

		public:
			virtual void OnDestroyed() = 0;
			virtual void OnPaint(HDC hdc) = 0;

		public:
			static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

		public:
			static GDIWindow* GetGDIWindow(HWND hWnd);

		public:
			static void RegisterWindowClass();
			static void RunDefaultMessageLoop();
			static void RunTestWindow(GDIWindow* window);
		};
	}
}
