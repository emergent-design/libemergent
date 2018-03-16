#pragma once

#include <entity/entity.hpp>


namespace emergent
{
	namespace profile
	{
		/// Running tallies and the final results for a single profiling point.
		struct AggregateStatistics
		{
			long mean 		= 0;
			long stddev		= 0;
			long minimum	= 0;
			long maximum	= 0;
			long samples	= 0;

			long sum		= 0;
			long squared	= 0;


			void Add(long reading)
			{
				this->minimum = this->samples ? std::min(this->minimum, reading) : reading;
				this->maximum = this->samples ? std::max(this->maximum, reading) : reading;

				this->sum		+= reading;
				this->squared	+= reading * reading;
				this->samples++;
			}


			void Reset()
			{
				this->samples	= this->sum		= this->squared	= 0;
				this->minimum	= this->maximum	= this->mean	= this->stddev = 0;
			}


			void Update()
			{
				if (this->samples)
				{
					this->mean		= this->sum / this->samples;
					this->stddev	= lrint(sqrt((this->squared / this->samples) - this->mean * this->mean));
				}
			}


			emap(eref(mean), eref(stddev), eref(minimum), eref(maximum), eref(samples))
		};


		/// Contains all of the results for this process.
		struct Aggregation
		{
			// The unique ID for this process that the Aggregator was initialised with
			std::string id;

			// Each entry in the map contains the results for a single named profiling
			// point. Good namespacing in profile names is recommended to avoid
			// accidental corruption of results.
			std::map<std::string, AggregateStatistics> statistics;


			/// Generate a current snapshot of the statistics for all entries.
			Aggregation &Snapshot()
			{
				for (auto &s : this->statistics) s.second.Update();
				return *this;
			}


			/// Reset the statistics for all entries so that tallying may
			/// start again.
			void Reset()
			{
				for (auto &s : this->statistics) s.second.Reset();
			}


			emap(eref(id), eref(statistics))
		};
	}
}
