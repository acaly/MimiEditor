#pragma once
#include <memory>

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
}
