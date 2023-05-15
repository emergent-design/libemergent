#pragma once

#include <ctime>
#include <emergent/String.hpp>


namespace emergent
{
	// UTC timestamp
	struct DateTime
	{
		time_t timestamp = -1;

		DateTime() {}

		DateTime(const time_t &time) : timestamp(time) {}
		DateTime(const DateTime &time) : timestamp(time.timestamp) {}
		DateTime(const tm &time) : timestamp(std::mktime(const_cast<struct tm*>(&time))) {}


		static DateTime Now()
		{
			return time(0);
		}


		static DateTime Today()
		{
			auto t		= Convert(time(0));
			t.tm_hour	= t.tm_min = t.tm_sec = 0;

			return mktime(&t);
		}


		static DateTime FromISO(std::string &time)
		{
			tm t 			= {};
			auto *remaining = strptime(time.c_str(), "%Y-%m-%dT%H:%M:%SZ", &t);

			return remaining && !*remaining ? mktime(&t) : -1;
		}


		// Convert a timestamp to calendar time. Useful for formatting a string
		static tm Convert(const time_t timestamp)
		{
			tm t;
			gmtime_r(&timestamp, &t);
			return t;
		}


		std::string FormatISO() const
		{
			const auto t = Convert(this->timestamp);

			return String::format(
				"%04d-%02d-%02dT%02d:%02d:%02dZ",
				1900+t.tm_year, 1+t.tm_mon, t.tm_mday,
				t.tm_hour, t.tm_min, t.tm_sec
			);
		}


		std::string FormatDateISO() const
		{
			const auto t = Convert(this->timestamp);

			return String::format(
				"%04d-%02d-%02d",
				1900 + t.tm_year, 1 + t.tm_mon, t.tm_mday
			);
		}


		// Provides the year and month in ISO format: YYYY-MM
		std::string FormatMonthISO() const
		{
			const auto t = Convert(this->timestamp);

			return String::format("%04d-%02d", 1900 + t.tm_year, 1 + t.tm_mon);
		}


		bool operator <(const DateTime &time) const		{ return this->timestamp < time.timestamp; }
		bool operator >(const DateTime &time) const		{ return this->timestamp > time.timestamp; }
		bool operator ==(const DateTime &time) const	{ return this->timestamp == time.timestamp; }
		bool operator !=(const DateTime &time) const	{ return this->timestamp != time.timestamp; }
		bool operator <=(const DateTime &time) const	{ return this->timestamp <= time.timestamp; }
		bool operator >=(const DateTime &time) const	{ return this->timestamp >= time.timestamp; }


		DateTime &AddSeconds(int seconds)	{ this->timestamp += seconds;			return *this; }
		DateTime &AddMinutes(int minutes)	{ this->timestamp += 60 * minutes;		return *this; }
		DateTime &AddHours(int hours)		{ this->timestamp += 3600 * hours;		return *this; }
		DateTime &AddDays(int days)			{ this->timestamp += 24 * 3600 * days;	return *this; }

		DateTime &AddMonths(int months)
		{
			auto t			 = Convert(this->timestamp);
			t.tm_mon		+= months;
			this->timestamp  = mktime(&t);
			return *this;
		}

		DateTime &AddYears(int years)
		{
			auto t			 = Convert(this->timestamp);
			t.tm_year		+= years;
			this->timestamp  = mktime(&t);
			return *this;
		}
	};
}
