#include "../MimiEditor/String.h"
using namespace Mimi;

extern Mimi::CodePage GetWindowsCodePage936();

int TestEncodingString()
{
	String a = String::FromUtf8(u8"你好123");
	String b = String::FromUtf16(u"你好123");
	String c = a.ToUtf16String();
	String d = b.ToUtf8String();
	unsigned char hello[] = { 0xC4, 0xE3, 0xBA, 0xC3, 0x00 };
	String e = String(reinterpret_cast<std::uint8_t*>(hello),
		sizeof(hello), GetWindowsCodePage936());
	String f = e.ToUtf16String();
	return 0;
}
