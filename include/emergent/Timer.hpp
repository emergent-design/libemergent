#pragma once

#include <chrono>


namespace emergent
{
	using namespace std::chrono;


	class Timer
	{
		public:

			// Returns the number of milliseconds that have elapsed since construction or reset.
			long Elapsed()
			{
				return duration_cast<milliseconds>(steady_clock::now() - this->time).count();
			}

			// Returns the number of microseconds that have elapsed since construction or reset.
			long MicroElapsed()
			{
				return duration_cast<microseconds>(steady_clock::now() - this->time).count();
			}

			// Resets the reference time to now.
			void Reset()
			{
				this->time = steady_clock::now();
			}

		private:

			time_point<steady_clock> time = steady_clock::now();
	};
}
