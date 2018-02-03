#include "Clock.h"
#include <Windows.h>
#include <cassert>

std::uint64_t Mimi::Clock::GetTime()
{
	LARGE_INTEGER r;
	assert(::QueryPerformanceCounter(&r));
	return static_cast<std::uint64_t>(r.QuadPart);
}

std::uint64_t Mimi::Clock::GetMilliSecondLength()
{
	LARGE_INTEGER r;
	assert(::QueryPerformanceFrequency(&r));
	return static_cast<std::uint64_t>(r.QuadPart / 1000);
}
