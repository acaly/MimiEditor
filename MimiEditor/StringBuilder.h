#pragma once
#include "String.h"
#include "Buffer.h"

namespace Mimi
{
	namespace StringFormatter
	{
		static void ReverseUtf8(DynamicBuffer& buffer)
		{
			mchar8_t* data = buffer.GetPointer();
			std::size_t start = 0;
			std::size_t end = buffer.GetLength() - 1;
			while (start < end)
			{
				mchar8_t ch = data[start];
				data[start] = data[end];
				data[end] = ch;
				++start;
				--end;
			}
		}

		template <typename T>
		struct ValueFormatter;

		template <typename T>
		struct ValueFormatterUInt
		{
		public:
			static String Format(DynamicBuffer& buffer, const T& val)
			{
				if (val == 0) return String::FromUtf8("0");
				T v = val;

				buffer.Clear();
				do
				{
					T i = v % 10;
					v /= 10;
					mchar8_t append = '0' + static_cast<std::uint8_t>(i);
					buffer.Append(&append, 1);
				} while (v > 0);
				ReverseUtf8(buffer);

				return String(buffer.GetRawData(), buffer.GetLength(), CodePageManager::UTF8);
			}

			static String Format(DynamicBuffer& buffer, const T& val, const String& str)
			{
				return Format(buffer, val);
			}
		};

		template <typename T>
		struct MaximumSIntString;

		template <> struct MaximumSIntString<int>
		{
		public:
			static int NegativeMax() { return -2147483648LL; }
			static const char* NegativeMaxString() { return "-2147483648"; }
		};

		template <typename T>
		struct ValueFormatterSInt
		{
		public:
			static String Format(DynamicBuffer& buffer, const T& val)
			{
				if (val == 0) return String::FromUtf8("0");
				if (val == MaximumSIntString<T>::NegativeMax())
				{
					return String::FromUtf8Ptr(MaximumSIntString<T>::NegativeMaxString());
				}
				if (val < 0) return Format(buffer, -val);
				T v = val;

				buffer.Clear();
				do
				{
					T i = v % 10;
					v /= 10;
					mchar8_t append = '0' + static_cast<std::uint8_t>(i);
					buffer.Append(&append, 1);
				} while (v > 0);
				ReverseUtf8(buffer);

				return String(buffer.GetRawData(), buffer.GetLength(), CodePageManager::UTF8);
			}

			static String Format(DynamicBuffer& buffer, const T& val, const String& str)
			{
				return Format(buffer, val);
			}
		};

		template<> struct ValueFormatter<std::size_t> : ValueFormatterUInt<std::size_t> {};
		template<> struct ValueFormatter<int> : ValueFormatterSInt<int> {};
	}

	class StringBuilder
	{
	public:
		StringBuilder(CodePage encoding)
			: Buffer(100), FormatBuffer(100), Encoding(encoding)
		{
		}

	private:
		CodePage Encoding;
		DynamicBuffer Buffer;
		DynamicBuffer FormatBuffer;

	public:
		void AppendUtf8(const char* str)
		{
			Append(String::FromUtf8Ptr(str));
		}

		void Append(const String& str)
		{
			if (str.Encoding != Encoding)
			{
				Append(str.ToCodePage(Encoding));
			}
			Buffer.Append(str.Data, str.Length - Encoding.GetNormalWidth()); //Trim the '\0'
		}

		template <typename T>
		void AppendValue(const T& val)
		{
			Append(StringFormatter::ValueFormatter<T>::Format(FormatBuffer, val));
		}

		String ToString() const
		{
			return String(Buffer.GetRawData(), Buffer.GetLength(), Encoding);
		}

		void CopyTo(DynamicBuffer& buffer)
		{
			buffer.Clear();
			buffer.Append(Buffer.GetRawData(), Buffer.GetLength());
		}

		//TODO support format string
	};
}
