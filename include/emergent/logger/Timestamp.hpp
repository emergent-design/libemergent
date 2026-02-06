#pragma once

// #include <time.h>
#include <ctime>
#include <chrono>
#include <emergent/String.hpp>


namespace emergent
{
	class Timestamp
	{
		public:

			static inline tm Time()
			{
				return From(std::time(0));
			}


			static inline tm From(time_t time)
			{
				tm result;

				#if defined(_WIN64) || defined(_WIN32)
					result = *localtime(&time);
				#else
					localtime_r(&time, &result);
				#endif

				return result;
			}


			/// Produce a string timestamp
			static const std::string Now()
			{
				tm t = Time();
				return String::format(
					"%04d-%02d-%02d %02d:%02d:%02d",
					1900 + t.tm_year, 1 + t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec
				);
			}


			/// Produce a string timestamp suitable for filenames
			static const std::string FNow()
			{
				tm t = Time();
				return String::format(
					"%04d-%02d-%02d_%02d%02d%02d",
					1900 + t.tm_year, 1 + t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec
				);
			}


			/// Produce a string of todays date
			static const std::string Date()
			{
				tm t = Time();
				return String::format(
					"%04d-%02d-%02d",
					1900 + t.tm_year, 1 + t.tm_mon, t.tm_mday
				);
			}


			/// A millisecond precision time point
			static uint64_t LogTime()
			{
				return std::chrono::duration_cast<std::chrono::milliseconds>(
					std::chrono::high_resolution_clock::now().time_since_epoch()
				).count();
			}
	};
}
