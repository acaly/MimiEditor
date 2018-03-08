#include "GDIWindow.h"

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

namespace
{
	HINSTANCE GetHInstance()
	{
		HMODULE hModule;
		::GetModuleHandleEx(
			GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			reinterpret_cast<LPCWSTR>(GetHInstance), &hModule);
		return reinterpret_cast<HINSTANCE>(hModule);
	}

	const HINSTANCE AppInstance = GetHInstance();

	const LPCWSTR WindowClassName = L"MIMI_GDI_WINDOW";
	const LPCWSTR TestWindowWindowClassName = L"MIMI_GDI_WINDOW_TESTWINDOW";

	bool ControlClassRegistered = false;
	bool TestWindowClassRegistered = false;

	LRESULT CALLBACK TestWindowWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);

			RECT range = ps.rcPaint;
			auto hBrush = CreateSolidBrush(RGB(255, 255, 255));
			FillRect(hdc, &range, hBrush);
			DeleteObject(hBrush);
			EndPaint(hWnd, &ps);
			return 0;
		}
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		default:
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}
	}

	void RegisterControlClass(WNDPROC wndProc)
	{
		WNDCLASS wc = {};

		//TODO wc.style?
		wc.lpfnWndProc = wndProc;
		wc.hInstance = AppInstance;
		wc.lpszClassName = WindowClassName;
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

		RegisterClass(&wc);

		ControlClassRegistered = true;
	}

	void RegisterTestWindowClass()
	{
		WNDCLASS wc = {};

		wc.lpfnWndProc = TestWindowWindowProc;
		wc.hInstance = AppInstance;
		wc.lpszClassName = TestWindowWindowClassName;
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

		RegisterClass(&wc);

		TestWindowClassRegistered = true;
	}
}

void Mimi::GDI::GDIWindow::CreateControlWindow(HWND parent, RECT position)
{
	Size = { position.right - position.left, position.bottom - position.top };
	HWnd = CreateWindowEx(
		0, WindowClassName, L"",
		WS_TABSTOP | WS_VISIBLE | WS_CHILD,
		position.left, position.top, Size.cx, Size.cy,
		parent,
		NULL, AppInstance, NULL);
}

LRESULT Mimi::GDI::GDIWindow::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);

		RECT range = ps.rcPaint;
		auto hBrush = CreateSolidBrush(RGB(0, 128, 255));
		FillRect(hdc, &range, hBrush);
		DeleteObject(hBrush);

		EndPaint(hWnd, &ps);
		return 0;
	}
	//TODO resize
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
}

void Mimi::GDI::GDIWindow::RegisterWindowClass()
{
	RegisterControlClass(WindowProc);
}

void Mimi::GDI::GDIWindow::RunDefaultMessageLoop()
{
	MSG msg = {};
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

void Mimi::GDI::GDIWindow::RunTestWindow(GDIWindow* window)
{
	if (!TestWindowClassRegistered)
	{
		RegisterTestWindowClass();
	}
	assert(ControlClassRegistered);

	HWND hWndWindow = CreateWindowEx(
		0, TestWindowWindowClassName, L"Mimi GDI Control Test",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL,
		NULL, AppInstance, NULL);

	window->CreateControlWindow(hWndWindow, { 0, 0, 200, 200 });

	ShowWindow(hWndWindow, SW_SHOW);
	SetFocus(window->GetHWnd());

	RunDefaultMessageLoop();
}
