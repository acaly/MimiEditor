#include "../MimiEditor/String.h"
using namespace Mimi;

int TestEncodingString()
{
	String a = String::FromUtf8(u8"���123");
	String b = String::FromUtf16(u"���123");
	String c = a.ToUtf16String();
	String d = b.ToUtf8String();
	return 0;
}
