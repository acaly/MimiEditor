#include "TestCommon.h"
#include "../MimiEditor/String.h"
using namespace Mimi;

extern Mimi::CodePage GetWindowsCodePage936();

namespace
{
	class CodePageTester
	{
	public:
		CodePageTester(lest::env& lest_env, CodePage cp)
			: lest_env(lest_env), CodePageObj(cp)
		{
		}

	private:
		lest::env& lest_env;
		CodePage CodePageObj;

	public:
		void TestBasic()
		{
			EXPECT(CodePageObj.IsValid());
			EXPECT(CodePageObj.GetNormalWidth() > 0);
			EXPECT(CodePageObj.GetNormalWidth() <= 4);
		}

		void TestNormalChar(char ch)
		{
			mchar8_t buffer[4] = {};
			char32_t ch32 = ch;
			auto lenFrom32 = CodePageObj.CharFromUTF32(ch32, buffer);
			EXPECT(lenFrom32 == CodePageObj.GetNormalWidth());
			auto lenTo32 = CodePageObj.CharToUTF32(buffer, &ch32);
			EXPECT(lenTo32 == CodePageObj.GetNormalWidth());
			EXPECT(ch32 == ch);
		}

		void TestChar(char32_t chIn)
		{
			mchar8_t buffer[4] = {};
			auto lenFrom32 = CodePageObj.CharFromUTF32(chIn, buffer);
			char32_t chOut;
			auto lenTo32 = CodePageObj.CharToUTF32(buffer, &chOut);
			EXPECT(lenFrom32 > 0);
			EXPECT(lenFrom32 <= 4);
			EXPECT(lenFrom32 == lenTo32);
			EXPECT(chOut == chIn);
		}

		void TestInvalidChar(char32_t chIn)
		{
			mchar8_t buffer[4] = {};
			auto lenFrom32 = CodePageObj.CharFromUTF32(chIn, buffer);
			EXPECT(lenFrom32 == 0);
		}

		void TestString(const char* u8)
		{
			String u8String = String::FromUtf8Ptr(u8);
			String cpString = u8String.ToCodePage(CodePageObj);
			String u8StringOut = cpString.ToUtf8String();
			const char* u8Out = reinterpret_cast<const char*>(u8StringOut.Data);
			EXPECT(std::strcmp(u8, u8Out) == 0);
		}

		template <std::size_t N>
		void TestCharList(const char32_t(&list)[N])
		{
			for (std::size_t i = 0; i < N; ++i)
			{
				TestChar(list[i]);
			}
		}

		template <std::size_t N>
		void TestInvalidCharList(const char32_t(&list)[N])
		{
			for (std::size_t i = 0; i < N; ++i)
			{
				TestInvalidChar(list[i]);
			}
		}
	};
}

DEFINE_MODULE(TestEncodingString)
{
	CASE("UTF8")
	{
		CodePageTester tester = { lest_env, CodePageManager::UTF8 };
		tester.TestBasic();
		tester.TestNormalChar('\n');
		tester.TestNormalChar('\0');
		tester.TestCharList({ U'\x41', U'\x7A' });
		tester.TestCharList({ U'\xA3', U'\xC0', U'\xF7' });
		tester.TestCharList({ U'\x20AC', U'\x3042', U'\x3231', U'\x4F60', U'\x597D' });
		tester.TestCharList({ U'\x10082', U'\x1F34C', U'\x1F600', U'\x24B62' });
		tester.TestCharList({ U'\x2F80F', U'\xE0041', U'\x101234' });

		tester.TestCharList({ U'\x7F', U'\x80', U'\x7FF', U'\x800', U'\xFFFF' });
		tester.TestCharList({ U'\x10000', U'\x10FFFF' });
		tester.TestInvalidCharList({ U'\x110000', U'\x12EDCB' });

		tester.TestString(u8"");
		tester.TestString(u8"ABC");
		tester.TestString(u8"ABC\xA3\x20AC\x597D\x10082\x2F80F\x101234");
	},
	CASE("UTF16 - LE")
	{
		CodePageTester tester = { lest_env, CodePageManager::UTF16LE };
		tester.TestBasic();
		tester.TestNormalChar('\n');
		tester.TestNormalChar('\0');
		tester.TestCharList({ U'\x41', U'\x7A' });
		tester.TestCharList({ U'\xA3', U'\xC0', U'\xF7' });
		tester.TestCharList({ U'\x20AC', U'\x3042', U'\x3231', U'\x4F60', U'\x597D' });
		tester.TestCharList({ U'\x10082', U'\x1F34C', U'\x1F600', U'\x24B62' });
		tester.TestCharList({ U'\x2F80F', U'\xE0041', U'\x101234' });

		tester.TestCharList({ U'\xD7FF', U'\xE000', U'\xFFFF', U'\x10000', U'\x10FFFF' });
		tester.TestInvalidCharList({ U'\x110000', U'\x12EDCB' });

		tester.TestString(u8"");
		tester.TestString(u8"ABC");
		tester.TestString(u8"ABC\xA3\x20AC\x597D\x10082\x2F80F\x101234");
	},
	CASE("UTF16 - BE")
	{
		CodePageTester tester = { lest_env, CodePageManager::UTF16BE };
		tester.TestBasic();
		tester.TestNormalChar('\n');
		tester.TestNormalChar('\0');
		tester.TestCharList({ U'\x41', U'\x7A' });
		tester.TestCharList({ U'\xA3', U'\xC0', U'\xF7' });
		tester.TestCharList({ U'\x20AC', U'\x3042', U'\x3231', U'\x4F60', U'\x597D' });
		tester.TestCharList({ U'\x10082', U'\x1F34C', U'\x1F600', U'\x24B62' });
		tester.TestCharList({ U'\x2F80F', U'\xE0041', U'\x101234' });

		tester.TestCharList({ U'\xD7FF', U'\xE000', U'\xFFFF', U'\x10000', U'\x10FFFF' });
		tester.TestInvalidCharList({ U'\x110000', U'\x12EDCB' });

		tester.TestString(u8"");
		tester.TestString(u8"ABC");
		tester.TestString(u8"ABC\xA3\x20AC\x597D\x10082\x2F80F\x101234");
	},
	CASE("Windows 936")
	{
		CodePageTester tester = { lest_env, GetWindowsCodePage936() };
		tester.TestBasic();
		tester.TestNormalChar('\n');
		tester.TestNormalChar('\0');
		tester.TestCharList({ U'\x41', U'\x7A' });
		tester.TestCharList({ U'\x4F60', U'\x597D' });

		tester.TestString(u8"");
		tester.TestString(u8"ABC");
		tester.TestString(u8"ABC\x4F60\x597D");
	}
};
