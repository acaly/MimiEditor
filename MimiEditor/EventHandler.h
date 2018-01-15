#pragma once
#include <memory>
#include <unordered_map>
#include <queue>

namespace Mimi
{
	namespace EventHandlers
	{
		template <typename T>
		struct IEventHandler_
		{
			virtual ~IEventHandler_() {}
			virtual void Invoke(T* e) = 0;
		};

		template <typename T>
		struct RawFunctionPointer_
		{
			typedef void(*Normal)(T* e);
			typedef void(*WithUserData)(T* e, void* ud);
		};

		template <typename C, typename T>
		struct MemberFunctionPointer_
		{
			typedef void(C::*Normal)(T* e);
		};

		template <typename T>
		using RawFunctionPointer = typename RawFunctionPointer_<T>::Normal;

		template <typename T>
		using RawFunctionPointerWithUserData = typename RawFunctionPointer_<T>::WithUserData;

		template <typename C, typename T>
		using MemberFunctionPointer = typename MemberFunctionPointer_<C, T>::Normal;

		template <typename T>
		struct RawFunctionPointerHandler : IEventHandler_<T>
		{
			RawFunctionPointerHandler(RawFunctionPointer<T> f)
			{
				Function = f;
			}

			virtual void Invoke(T* e)
			{
				Function(e);
			}

		private:
			RawFunctionPointer<T> Function;
		};

		template <typename T>
		struct RawFunctionPointerWithUserDataHandler : IEventHandler_<T>
		{
			RawFunctionPointerWithUserDataHandler(RawFunctionPointerWithUserData<T> f, void* ud)
			{
				Function = f;
				UserData = ud;
			}

			virtual void Invoke(T* e)
			{
				Function(e, UserData);
			}

		private:
			RawFunctionPointerWithUserData<T> Function;
			void* UserData;
		};

		template <typename C, typename T>
		struct MemberFunctionHandler : IEventHandler_<T>
		{
			MemberFunctionHandler(MemberFunctionPointer<C, T> f, C* ins)
			{
				Function = f;
				Instance = ins;
			}

			virtual void Invoke(T* e)
			{
				(Instance->*Function)(e);
			}

			MemberFunctionPointer<C, T> Function;
			C* Instance;
		};
	}

	template <typename T>
	using IEventHandler = typename std::unique_ptr<EventHandlers::IEventHandler_<T>>;

	class EventHandler final
	{
		EventHandler() {}

	public:
		template <typename T>
		static IEventHandler<T> FromStatic(void (*f)(T*))
		{
			return std::unique_ptr<EventHandlers::IEventHandler_<T>>
				(new EventHandlers::RawFunctionPointerHandler<T>(f));
		}

		template <typename T>
		static IEventHandler<T> FromStatic(void (*f)(T*, void*), void* ud)
		{
			return std::unique_ptr<EventHandlers::IEventHandler_<T>>
				(new EventHandlers::RawFunctionPointerWithUserDataHandler<T>(f, ud));
		}

		template <typename T, typename C>
		static IEventHandler<T> FromMember(void (C::*f)(T*), C* i)
		{
			return std::unique_ptr<EventHandlers::IEventHandler_<T>>
				(new EventHandlers::MemberFunctionHandler<C, T>(f, i));
		}
	};

	typedef unsigned int HandlerId;

	//TODO multithread?
	template <typename T>
	class Event final
	{
	public:
		Event() = default;
		Event(const Event&) = delete;
		Event(Event&&) = delete;
		Event& operator= (const Event&) = delete;
		~Event() = default;

	private:
		HandlerId NextId;
		std::unordered_map<HandlerId, IEventHandler<T>> Handlers;
		std::queue<HandlerId> UnusedId;

	private:
		HandlerId GetFreeId()
		{
			if (UnusedId.empty())
			{
				const HandlerId Allocate = 10;
				for (HandlerId i = 1; i < Allocate; ++i) //0 is returned
				{
					UnusedId.push(NextId + i);
				}
				HandlerId ret = NextId;
				NextId += Allocate;
				return ret;
			}
			else
			{
				HandlerId ret = UnusedId.front(); //I hate C++
				UnusedId.pop();
				return ret;
			}
		}

	public:
		HandlerId AddHandler(IEventHandler<T> h)
		{
			HandlerId ret = GetFreeId();
			Handlers[ret] = std::move(h);
			return ret;
		}

		void RemoveHandler(HandlerId h)
		{
			Handlers.erase(h);
			UnusedId.push(h);
		}

		void InvokeAll(T* e)
		{
			for (auto& entry : Handlers)
			{
				entry.second->Invoke(e);
			}
		}
	};
}
