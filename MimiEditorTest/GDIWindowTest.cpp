#include "../MimiEditor/GDIWindow.h"

using namespace Mimi;
using namespace Mimi::GDI;

namespace
{
	class EmptyGDIWindow : public GDIWindow
	{
		virtual void OnDestroyed() override
		{
		}

		virtual void OnPaint(HDC hdc) override
		{
		}
	};
}

void TestGDIWindow()
{
	GDIWindow::RegisterWindowClass();
	GDIWindow* window = new EmptyGDIWindow();
	GDIWindow::RunTestWindow(window);
}
