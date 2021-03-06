#pragma once

#include <emergent/concurrentqueue.h>
#include <emergent/profile/Storage.hpp>

namespace emergent
{
	namespace profile
	{
		class Aggregator
		{
			public:

				/// Accessor for the singleton instance
				inline static Aggregator &Instance()
				{
					static Aggregator _instance;
					return _instance;
				}


				/// Initialise with a process specific ID (to prevent overwriting of statistics
				/// generated by multiple applications using the profiler). Specify the type of
				/// storage required and how long between aggregations (ms).
				bool Initialise(std::string id, std::unique_ptr<Storage> storage, long poll = 1000)
				{
					if (!this->run)
					{
						this->id				= id;
						this->storage			= std::move(storage);
						this->poll				= poll;
						this->aggregation.id	= id;

						if (this->storage->Initialise())
						{
							this->run		= true;
							this->thread	= std::thread(&Aggregator::Entry, this);

							return true;
						}
					}

					return false;
				}


				/// Push a single reading to the aggregrator. This is pushed onto a concurrent
				/// queue to try and avoid situations where the profiler itself is impeding
				/// performance.
				void Push(std::string name, long time)
				{
					if (this->run) this->queue.enqueue({ name, time });
				}


			private:

				/// Simple structure used for queueing individual readings
				struct Reading
				{
					std::string name;	// Unique name for this reading
					long time;			// Time taken in us
				};

				// Prevent instantiation
				Aggregator()
				{
					this->run = false;
				}

				// Prevent copy construction
				Aggregator(Aggregator const&) {}

				// Prevent assignment
				Aggregator& operator=(Aggregator const&)
				{
					return *this;
				}

				~Aggregator()
				{
					if (this->run)
					{
						this->run = false;
						this->thread.join();
					}
				}

				void Entry()
				{
					using namespace std::chrono;

					Reading reading;
					auto last = steady_clock::now();

					while (this->run)
					{
						if (duration_cast<milliseconds>(steady_clock::now() - last).count() > this->poll)
						{
							this->storage->Store(this->aggregation.Snapshot());
							this->aggregation.Reset();

							last = steady_clock::now();
						}

						// Only sleep if there is nothing in the queue
						if (this->queue.try_dequeue(reading))
						{
							this->aggregation.statistics[reading.name].Add(reading.time);
						}
						else std::this_thread::sleep_for(milliseconds(10));
					}

					this->storage->Close();
				}


				moodycamel::ConcurrentQueue<Reading> queue;	// Temporary buffer for the incoming data
				std::unique_ptr<Storage> storage;			// Where to write the aggregations
				Aggregation aggregation;					// Current aggregated results
				std::string id;								// Name of this application (aggregator is a singleton, so one instance per process)
				long poll;									// Poll time for aggregating results (ms)

				std::thread thread;							// Thread on which to generate and store the results
				std::atomic<bool> run;
		};
	}
}
