#pragma once
#include "String.h"
#include <cassert>

namespace Mimi
{
	namespace ResultImpl
	{
		class Error
		{
		public:
			//No destructor because Error objects should not be deleted during
			//execution.
			virtual String GetErrorMessage() const = 0;
			virtual const char* GetErrorId() const = 0;
		};

		class SimpleError : public Error
		{
		public:
			constexpr SimpleError(const char* msg)
				: Msg(msg)
			{
			}

		private:
			const char* const Msg;

		public:
			virtual String GetErrorMessage() const override
			{
				return String::FromUtf8Ptr(Msg);
			}

			virtual const char* GetErrorId() const override
			{
				return Msg;
			}
		};

		struct ResultBase
		{
			const Error* ErrorPtr;

			bool Success()
			{
				return !ErrorPtr;
			}

			bool Failed()
			{
				return ErrorPtr;
			}

			String GetErrorMessage()
			{
				assert(ErrorPtr);
				return ErrorPtr->GetErrorMessage();
			}

			const char* GetErrorId()
			{
				if (ErrorPtr)
				{
					return ErrorPtr->GetErrorId();
				}
				return nullptr;
			}

			bool IsError(const char* id)
			{
				return ErrorPtr && std::strcmp(GetErrorId(), id) == 0;
			}
		};

		template <typename T>
		struct Result : ResultBase
		{
			T Value;

			operator T()
			{
				return Value;
			}

			template <typename TE, typename = std::enable_if<std::is_base_of<Error, TE>::value>::type>
			Result(TE& error)
			{
				ErrorPtr = &error;
				Value = T();
			}

			Result(T&& val)
			{
				ErrorPtr = nullptr;
				Value = val;
			}

			template <typename TO>
			Result(const Result<TO>& err)
			{
				assert(err.Failed());
				ErrorPtr = err.ErrorPtr;
				Value = T();
			}

			template <typename = std::enable_if<std::is_pointer<T>::value>::type>
			T operator->()
			{
				return Value;
			}
		};

		template <>
		struct Result<void> : ResultBase
		{
			Result(bool r)
			{
				assert(r);
				ErrorPtr = nullptr;
			}

			template <typename TE, typename = std::enable_if<std::is_base_of<Error, TE>::value>::type>
			Result(TE& error)
			{
				ErrorPtr = &error;
			}

			template <typename TO>
			Result(const Result<TO>& err)
			{
				assert(err.Failed());
				ErrorPtr = err.ErrorPtr;
			}
		};
	}

	template <typename T = void>
	using Result = ResultImpl::Result<T>;

#define DECLARE_ERROR_CODE_SIMPLE(name) constexpr ResultImpl::SimpleError name = #name

	namespace ErrorCodes
	{
		//TODO support error message storage (in thread local)
		DECLARE_ERROR_CODE_SIMPLE(Unknown);

		DECLARE_ERROR_CODE_SIMPLE(NotImplemented);
		DECLARE_ERROR_CODE_SIMPLE(InvalidArgument);
	}
}
