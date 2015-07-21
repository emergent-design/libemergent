#include "emergent/profile/Aggregator.h"

using namespace std;
using namespace std::chrono;

namespace emergent
{
	namespace profile
	{
		Aggregator& Aggregator::Instance()
		{
			static Aggregator _instance;
			return _instance;
		}


		Aggregator::Aggregator()
		{
			this->run = false;
		}


		Aggregator::Aggregator(Aggregator const&)
		{
		}


		Aggregator::~Aggregator()
		{
			this->run = false;
			this->thread.join();
		}


		Aggregator& Aggregator::operator=(Aggregator const&)
		{
			return *this;
		}


		bool Aggregator::Initialise(string id, unique_ptr<Storage> storage, long poll)
		{
			if (!this->run)
			{
				this->id				= id;
				this->storage			= move(storage);
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


		void Aggregator::Push(string name, long time)
		{
			if (this->run) this->queue.enqueue({ name, time });
		}


		void Aggregator::Entry()
		{
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
				else this_thread::sleep_for(milliseconds(10));
			}

			this->storage->Close();
		}
	}
}

