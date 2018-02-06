#include "TestCommon.h"

MODULE_LIST(AllTests,
	TestModificationTracer,
	TestEventHandler);

int main()
{
	int ret = 0;
	for (auto&& m : AllTests)
	{
		ret += m.Run();
	}
	return ret;
}
