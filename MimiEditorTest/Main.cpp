#include "TestCommon.h"

MODULE_LIST(AllTests,
	TestModificationTracer,
	TestEventHandler,
	TestEncodingString,
	TestEncodingDetection,
	TestLineSeparation);

const char* ExecutableDirectory;

static void SetupExecutableDirectory(int argc, char** argv)
{
	assert(argc > 0);
	auto len = std::strlen(argv[0]);
	int sep = -1;
	for (int i = static_cast<int>(len - 1); i >= 0; --i)
	{
		auto ch = argv[0][i];
		if (ch == '/' || ch == '\\')
		{
			sep = i;
			break;
		}
	}
	if (sep == -1)
	{
		ExecutableDirectory = "";
	}
	else
	{
		auto path = new char[sep + 2];
		std::memcpy(path, argv[0], sep + 1);
		path[sep + 1] = 0;
		ExecutableDirectory = path;
	}
}

int main(int argc, char** argv)
{
	SetupExecutableDirectory(argc, argv);

	int ret = 0;
	for (auto&& m : AllTests)
	{
		ret += m.Run();
	}
	return ret;
}
