#pragma once

#include "lest.hpp"

struct TestModule
{
	const char* Name;
	lest::tests Tests;

	template <typename... T>
	TestModule(T... t)
	{
		lest::test a[] = { t... };
		Tests.insert(Tests.begin(), &a[0], &a[sizeof(a) / sizeof(lest::test)]);
	}

	TestModule(const char* name)
	{
		Name = name;
	}

	TestModule& operator += (TestModule&& other)
	{
		Tests = std::move(other.Tests);
		return *this;
	}

	int Run()
	{
		std::cout << "----------------------------------------------\n";
		std::cout << "Run test: " << Name << std::endl;
		std::cout << "----------------------------------------------\n";
		int ret = lest::run(Tests, { "-p" }, std::cout);
		std::cout << "----------------------------------------------\n\n";
		return ret;
	}
};

typedef TestModule GetTestModule();

struct GetTestModuleWrapper
{
	GetTestModuleWrapper(GetTestModule* func)
		: Func(func)
	{
	}

	int Run()
	{
		return Func().Run();
	}

private:
	GetTestModule* Func;
};

#define DEFINE_MODULE(name) \
	extern TestModule name ## _Value; \
	TestModule name() { return name ## _Value; } \
	TestModule name ## _Value = TestModule(#name) += TestModule

#define MODULE_LIST(name, ...) \
	extern GetTestModule __VA_ARGS__; \
	std::vector<GetTestModuleWrapper> name = { __VA_ARGS__ };
