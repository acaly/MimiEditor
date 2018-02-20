#pragma once

#include "lest.hpp"
#include "../MimiEditor/StringBuilder.h"
#include <cassert>
#include <cstring>
#include <vector>
#include <algorithm>
#include <functional>
#include <unordered_set>
#include <unordered_map>

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
		std::unordered_map<lest::text, int> counts;
		auto reporter = [&counts](const lest::message& e, const lest::text& t)
		{
			if (e.kind == "before test")
			{
				std::cout << t << std::endl;
			}
			else if (e.kind == "after test")
			{
				std::size_t maxWidth = 10;
				for (auto c : counts)
				{
					if (maxWidth < c.first.length()) maxWidth = c.first.length();
				}
				for (auto c : counts)
				{
					std::cout
						<< "  "
						<< std::left << std::setw(static_cast<std::streamsize>(maxWidth)) << c.first
						<< c.second << std::endl;
				}
				counts.clear();
			}
			else
			{
				counts[e.kind] += 1;
			}
		};

		std::cout << "----------------------------------------------\n";
		std::cout << "Run test: " << Name << std::endl;
		std::cout << "----------------------------------------------\n";

		int ret = lest::run(Tests, { }, std::cout, reporter);

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

extern const char* ExecutableDirectory;
