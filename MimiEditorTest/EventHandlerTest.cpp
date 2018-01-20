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
	int ud2 = 400;

	auto h1 = Mimi::EventHandler::FromStatic(&HandlerClass::StaticHandler);
	auto h2 = Mimi::EventHandler::FromStatic(&HandlerClass::StaticHandlerWithUD, &ud);
	auto h3 = Mimi::EventHandler::FromMember(&HandlerClass::MemberHandler, &i);
	auto h4 = Mimi::EventHandler::FromStatic(&HandlerClass::StaticHandlerWithUD, &ud2);

	Mimi::Event<EventArgs, int> handlers;
	handlers.AddHandler(std::move(h1), 1);
	auto id2 = handlers.AddHandler(std::move(h2));
	handlers.AddHandler(std::move(h3));
	auto id4 = handlers.AddHandler(std::move(h4));
	handlers.RemoveHandler(id2);

	handlers.InvokeAll(&e);

	std::cout << "-----" << std::endl;

	handlers.HandlerFilter(id4) = 1;
	handlers.InvokeWithFilter(&e, 1);

	return 0;
}
