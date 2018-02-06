#include "TestCommon.h"
#include "../MimiEditor/EventHandler.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>

using namespace Mimi;
using namespace std;

namespace
{
	struct EventArgs
	{
		int Value;
	};

	struct Callback
	{
		int Function;
		int Event;
		int Data;

		Callback(int f, int e, int d)
			: Function(f), Event(e), Data(d)
		{
		}

		bool operator == (const Callback& other)
		{
			return Function == other.Function &&
				Event == other.Event &&
				Data == other.Data;
		}
	};

	vector<Callback> CallbackRecords;

	struct HandlerClass
	{
		int MemberField;
		std::function<void()> Action;

		static void StaticHandler(EventArgs* arg)
		{
			CallbackRecords.push_back({ 1, arg->Value, 0 });
		}

		static void StaticHandlerWithUD(EventArgs* arg, void* ud)
		{
			CallbackRecords.push_back({ 2, arg->Value, *reinterpret_cast<int*>(ud) });
		}

		void MemberHandler(EventArgs* arg)
		{
			CallbackRecords.push_back({ 3, arg->Value, MemberField });
		}

		void MemberHandlerAction(EventArgs* arg)
		{
			CallbackRecords.push_back({ 4, arg->Value, MemberField });
			Action();
		}
	};

	bool CheckRecords(std::initializer_list<Callback> records)
	{
		for (auto&& r : records)
		{
			auto i = find(CallbackRecords.begin(), CallbackRecords.end(), r);
			if (i == CallbackRecords.end())
			{
				CallbackRecords.clear();
				return false;
			}
			CallbackRecords.erase(i);
		}
		if (CallbackRecords.size() != 0)
		{
			CallbackRecords.clear();
			return false;
		}
		return true;
	}
}

DEFINE_MODULE(TestEventHandler)
{
	CASE("Mixed handler")
	{
		Event<EventArgs, int> eventHub;
		EventArgs e = { 200 };
		int handler2 = 25000;
		HandlerClass handler3 = { 35000 }, handler4 = { 45000 };
		auto h1 = EventHandler::FromStatic(&HandlerClass::StaticHandler);
		eventHub.AddHandler(std::move(h1));
		eventHub.AddHandler(EventHandler::FromStatic(&HandlerClass::StaticHandlerWithUD, &handler2));
		eventHub.AddHandler(EventHandler::FromMember(&HandlerClass::MemberHandler, &handler3));
		eventHub.AddHandler(EventHandler::FromMember(&HandlerClass::MemberHandler, &handler4));
		eventHub.InvokeAll(&e);
		EXPECT(CheckRecords({ { 1, 200, 0 }, { 2, 200, 25000 }, { 3, 200, 35000 }, { 3, 200, 45000 } }));
	},
	CASE("Mixed handler (no filter)")
	{
		Event<EventArgs, void> eventHub;
		EventArgs e = { 200 };
		int handler2 = 25000;
		HandlerClass handler3 = { 35000 }, handler4 = { 45000 };
		auto h1 = EventHandler::FromStatic(&HandlerClass::StaticHandler);
		eventHub.AddHandler(std::move(h1));
		eventHub.AddHandler(EventHandler::FromStatic(&HandlerClass::StaticHandlerWithUD, &handler2));
		eventHub.AddHandler(EventHandler::FromMember(&HandlerClass::MemberHandler, &handler3));
		eventHub.AddHandler(EventHandler::FromMember(&HandlerClass::MemberHandler, &handler4));
		eventHub.InvokeAll(&e);
		EXPECT(CheckRecords({ { 1, 200, 0 },{ 2, 200, 25000 },{ 3, 200, 35000 },{ 3, 200, 45000 } }));
	},
	CASE("Filter")
	{
		Event<EventArgs, int> eventHub;
		EventArgs e = { 300 };
		int handler1 = 10000, handler2 = 20000;
		eventHub.AddHandler(EventHandler::FromStatic(&HandlerClass::StaticHandlerWithUD, &handler1), 1);
		eventHub.AddHandler(EventHandler::FromStatic(&HandlerClass::StaticHandlerWithUD, &handler2), 2);

		eventHub.InvokeAll(&e);
		EXPECT(CheckRecords({ { 2, 300, 10000 }, { 2, 300, 20000 } }));
		eventHub.InvokeWithFilter(&e, 1);
		EXPECT(CheckRecords({ { 2, 300, 10000 } }));
		eventHub.InvokeWithFilter(&e, 2);
		EXPECT(CheckRecords({ { 2, 300, 20000 } }));
	},
	CASE("Add or delete handler")
	{
		Event<EventArgs, int> eventHub;
		EventArgs e = { 100 };
		int handler1 = 10000, handler2 = 20000, handler3 = 30000;
		eventHub.AddHandler(EventHandler::FromStatic(&HandlerClass::StaticHandlerWithUD, &handler1));
		auto h2 = eventHub.AddHandler(EventHandler::FromStatic(&HandlerClass::StaticHandlerWithUD, &handler2));

		eventHub.InvokeAll(&e);
		EXPECT(CheckRecords({ { 2, 100, 10000 }, { 2, 100, 20000 } }));
		
		eventHub.AddHandler(EventHandler::FromStatic(&HandlerClass::StaticHandlerWithUD, &handler3));
		eventHub.InvokeAll(&e);
		EXPECT(CheckRecords({ { 2, 100, 10000 }, { 2, 100, 20000 }, { 2, 100, 30000 } }));

		eventHub.RemoveHandler(h2);
		eventHub.InvokeAll(&e);
		EXPECT(CheckRecords({ { 2, 100, 10000 }, { 2, 100, 30000 } }));
	},
	CASE("Add or delete handler (no filter)")
	{
		Event<EventArgs, void> eventHub;
		EventArgs e = { 100 };
		int handler1 = 10000, handler2 = 20000, handler3 = 30000;
		eventHub.AddHandler(EventHandler::FromStatic(&HandlerClass::StaticHandlerWithUD, &handler1));
		auto h2 = eventHub.AddHandler(EventHandler::FromStatic(&HandlerClass::StaticHandlerWithUD, &handler2));

		eventHub.InvokeAll(&e);
		EXPECT(CheckRecords({ { 2, 100, 10000 },{ 2, 100, 20000 } }));

		eventHub.AddHandler(EventHandler::FromStatic(&HandlerClass::StaticHandlerWithUD, &handler3));
		eventHub.InvokeAll(&e);
		EXPECT(CheckRecords({ { 2, 100, 10000 },{ 2, 100, 20000 },{ 2, 100, 30000 } }));

		eventHub.RemoveHandler(h2);
		eventHub.InvokeAll(&e);
		EXPECT(CheckRecords({ { 2, 100, 10000 },{ 2, 100, 30000 } }));
	},
	CASE("Change handler within handler")
	{
		Event<EventArgs, int> eventHub;
		EventArgs e = { 200 };
		HandlerClass handler1 = { 10000 }, handler2 = { 20000 };

		eventHub.AddHandler(EventHandler::FromStatic(&HandlerClass::StaticHandler), 1);
		auto h1 = EventHandler::FromMember(&HandlerClass::MemberHandlerAction, &handler1);
		auto id1 = eventHub.AddHandler(std::move(h1), 1);
		handler1.Action = [&eventHub, id1]()
		{
			eventHub.RemoveHandler(id1);
		};

		eventHub.InvokeAll(&e);
		EXPECT(CheckRecords({ { 1, 200, 0 }, { 4, 200, 10000 } })); //Delete

		eventHub.InvokeAll(&e);
		EXPECT(CheckRecords({ { 1, 200, 0 } }));

		auto h2 = EventHandler::FromMember(&HandlerClass::MemberHandlerAction, &handler2);
		auto id2 = eventHub.AddHandler(std::move(h2), 1);
		handler2.Action = [&eventHub, id2]()
		{
			eventHub.HandlerFilter(id2) = 2;
		};
		
		eventHub.InvokeWithFilter(&e, 2);
		EXPECT(CheckRecords({ }));

		eventHub.InvokeAll(&e);
		EXPECT(CheckRecords({ { 1, 200, 0}, { 4, 200, 20000 } })); //Change filter

		eventHub.InvokeWithFilter(&e, 1);
		EXPECT(CheckRecords({ { 1, 200, 0 } }));

		eventHub.InvokeWithFilter(&e, 2);
		EXPECT(CheckRecords({ { 4, 200, 20000 } }));

		eventHub.InvokeAll(&e);
		EXPECT(CheckRecords({ { 1, 200, 0 }, { 4, 200, 20000 } }));
	},
	CASE("Change handler within handler (no filter)")
	{
		Event<EventArgs, void> eventHub;
		EventArgs e = { 200 };
		HandlerClass handler1 = { 10000 }, handler2 = { 20000 };

		eventHub.AddHandler(EventHandler::FromStatic(&HandlerClass::StaticHandler));
		auto h1 = EventHandler::FromMember(&HandlerClass::MemberHandlerAction, &handler1);
		auto id1 = eventHub.AddHandler(std::move(h1));
		handler1.Action = [&eventHub, id1]()
		{
			eventHub.RemoveHandler(id1);
		};

		eventHub.InvokeAll(&e);
		EXPECT(CheckRecords({ { 1, 200, 0 },{ 4, 200, 10000 } })); //Delete

		eventHub.InvokeAll(&e);
		EXPECT(CheckRecords({ { 1, 200, 0 } }));
	},
};
