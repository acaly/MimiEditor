#include "../MimiEditor/GDIWindow.h"
#include "../MimiEditor/AbstractControl.h"

using namespace Mimi;
using namespace Mimi::GDI;

namespace
{
	class TestControl : public AbstractControl
	{
		virtual void OnCreated() override
		{
		}

		virtual void OnDestroyed() override
		{
		}

		virtual void OnPaint(Renderer* renderer) override
		{
			renderer->Clear(Color4I(0, 255, 128));
		}
	};
}

void TestGDIWindow()
{
	GDIWindow::RegisterWindowClass();

	TestControl ctrl;
	GDIWindow* window = new GDIWindow(&ctrl);
	GDIWindow::RunTestWindow(window);
}
