#pragma once
#include <cstdint>

namespace Mimi
{
	class Clock final
	{
	public:
		Clock()
			: StartTime(GetTime())
		{
		}

	public:
		const std::uint64_t StartTime;

	private:
		template <typename T>
		T GetElapsedUnit(std::uint64_t unit)
		{
			double ret = static_cast<double>(GetElapsedTime()) / static_cast<double>(unit);
			return static_cast<T>(ret);
		}

	public:
		std::uint64_t GetElapsedTime()
		{
			return GetTime() - StartTime;
		}

		template <typename T>
		T GetElapsedMilliSecond()
		{
			return GetElapsedUnit<T>(GetMilliSecondLength());
		}

		template <typename T>
		T GetElapsedSecond()
		{
			return GetElapsedUnit<T>(GetMilliSecondLength() * 1000);
		}

		template <typename T>
		T GetElapsedMinute()
		{
			return GetElapsedUnit<T>(GetMilliSecondLength() * 1000 * 60);
		}

	public:
		static std::uint64_t GetTime();
		static std::uint64_t GetMilliSecondLength();
	};
}
