#pragma once

#include <queue>
#include <emergent/thread/Persistent.hpp>


namespace emergent
{
	// A simple thread pool implementation. The size of the pool is determined
	// by the first template argument.
	template <std::size_t N> class ThreadPool
	{
		public:

			// The ThreadPool launches an additional thread to manage the queue.
			ThreadPool()
			{
				using namespace std::chrono;

				this->thread = std::thread([&]{

					std::unique_lock<std::mutex> lock(this->cs);

					while (this->run)
					{
						if (!this->queue.empty())
						{
							for (auto &t : this->threads)
							{
								if (t.Ready())
								{
									t.Assign(this->queue.front());
									this->queue.pop();
									break;
								}
							}

							// Allow this thread to sleep and items to be added to
							// the queue even if there are no available threads.
							this->condition.wait_for(lock, 10us);
						}
						else this->condition.wait(lock);
					}

				});
			}


			// Whilst the futures returned by Run() will not block, the destruction
			// of the pool will wait for all threads to complete their current tasks.
			~ThreadPool()
			{
				this->cs.lock();
					this->run = false;
				this->cs.unlock();
				this->condition.notify_one();
				this->thread.join();
			}


			// Run the job on a thread when available. It returns a future so that
			// you can wait for the job to finish and potentially retrieve the result,
			// but unlike with std::async the future does not automatically wait on
			// destruction which means you can fire-and-forget if necessary.
			template <typename T> auto Run(T &&job) -> std::future<decltype(job())>
			{
				std::lock_guard<std::mutex> lock(this->cs);

				if (this->queue.size() < QUEUE_MAX)
				{
					auto task = std::make_shared<internal::Task<decltype(job())>>(std::move(job));

					this->queue.push(task);
					this->condition.notify_one();

					return task->promise.get_future();
				}

				return {};
			}


		private:

			// Maximum queue size
			static const int QUEUE_MAX = 1024;

			// Threading members
			std::mutex cs;
			std::thread thread;
			std::condition_variable condition;
			bool run = true;

			// The pool of threads
			std::array<PersistentThread, N> threads;

			// The queue of tasks to be executed
			std::queue<std::shared_ptr<internal::TaskBase>> queue;
	};
}
