#include "../MimiEditor/EventHandler.h"
#include <iostream>

struct EventArgs
{
	int Value;
};

struct HandlerClass
{
	int MemberField;

	static void StaticHandler(EventArgs* arg)
	{
		std::cout << "Static: " << arg->Value << std::endl;
	}

	static void StaticHandlerWithUD(EventArgs* arg, void* ud)
	{
		std::cout << "StaticWithUD: " << arg->Value << ", " << *reinterpret_cast<int*>(ud) << std::endl;
	}

	void MemberHandler(EventArgs* arg)
	{
		std::cout << "Member: " << arg->Value << ", " << MemberField << std::endl;
	}
};

int TestEventHandler()
{
	HandlerClass i = { 100 };
	EventArgs e = { 200 };
	int ud = 300;

	auto h1 = Mimi::EventHandler::FromStatic(&HandlerClass::StaticHandler);
	auto h2 = Mimi::EventHandler::FromStatic(&HandlerClass::StaticHandlerWithUD, &ud);
	auto h3 = Mimi::EventHandler::FromMember(&HandlerClass::MemberHandler, &i);

	h1->Invoke(&e);
	h2->Invoke(&e);
	h3->Invoke(&e);

	return 0;
}