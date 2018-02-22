#include "TestCommon.h"

MODULE_LIST(AllTests,
	TestModificationTracer,
	TestEventHandler,
	TestEncodingString,
	TestEncodingDetection,
	TestLineSeparation,
	TestSegmentListModification);

//TODO Organize other test functions.
void TestReadLargeFile(const char* path);

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

struct ArgList
{
	struct Param1
	{
		bool HasValue;
		std::string Value;

		operator bool()
		{
			return HasValue;
		}

		const char* operator[] (int i)
		{
			if (i != 0) return {};
			return Value.c_str();
		}
	};

public:
	ArgList(int argc, char** argv)
		: Values(argv + 1, argv + argc)
	{
	}

public:
	std::vector<std::string> Values;

public:
	bool Has0(std::string val)
	{
		return std::find(Values.begin(), Values.end(), val) != Values.end();
	}

	Param1 Has1(std::string val)
	{
		auto pos = std::find(Values.begin(), Values.end(), val);
		std::size_t i = pos - Values.begin();
		if (i + 1 >= Values.size()) return { false };
		return { true, pos[1] };
	}
};

int main(int argc, char** argv)
{
	SetupExecutableDirectory(argc, argv);

	ArgList args(argc, argv);

	if (auto p = args.Has1("--TestReadLargeFile"))
	{
		TestReadLargeFile(p[0]);
		return 0;
	}

	if (args.Has0("--Last"))
	{
		AllTests.erase(AllTests.begin(), AllTests.end() - 1);
	}

	//Run tests
	int ret = 0;
	for (auto&& m : AllTests)
	{
		ret |= m.Run();
	}

	return ret;
}
