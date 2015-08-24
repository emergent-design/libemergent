#pragma once

#include <ctime>
#include <emergent/String.hpp>


namespace emergent
{
	class Timestamp
	{
		public:

			static inline tm Time()
			{
				tm result;
				time_t now = time(0);

				#ifdef __GNUC__
					localtime_r(&now, &result);
				#else
					localtime_s(&result, &now);
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
	};
}
