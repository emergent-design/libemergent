#pragma once

#include <atomic>
#include <thread>
#include <mutex>
#include <queue>
#include <future>
#include <functional>
#include <condition_variable>


namespace emergent
{
	namespace internal
	{
		// A base class allows the thread pool to queue tasks of arbitrary return type.
		struct TaskBase { virtual void Run() = 0; };

		// Task is separated out from the ThreadPool class since it requires
		// a specialisation to handle the "void" case for promise.
		template <typename T> struct Task : public TaskBase
		{
			std::promise<T> promise;
			std::function<T()> job;

			Task(std::function<T()> &&job) : job(std::move(job)) {}

			void Run()
			{
				this->promise.set_value(this->job());
			}
		};

		template <> void Task<void>::Run()
		{
			this->job();
			this->promise.set_value();
		}
	}


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
								if (!t.task)
								{
									t.Assign(this->queue.front());
									this->queue.pop();
									break;
								}
							}

							// Allow this thread to sleep and items to be added to
							// the queue even if there are no available threads.
							this->condition.wait_for(lock, 10ms);
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

			// Helper responsible for a single thread in the pool.
			struct Thread
			{
				std::mutex cs;
				std::thread thread;
				std::condition_variable condition;
				std::shared_ptr<internal::TaskBase> task;
				bool run = false;

				Thread()
				{
					using namespace std::chrono;

					this->thread = std::thread([&]{

						std::unique_lock<std::mutex> lock(this->cs);

						this->run = true;

						while (this->run)
						{
							if (this->task)
							{
								this->task->Run();
								this->task = nullptr;
							}

							this->condition.wait(lock);
						}
					});

					// Wait until the thread has control of the mutex
					while (!this->run)
					{
						std::this_thread::sleep_for(1us);
					}
				}


				~Thread()
				{
					this->cs.lock();
						this->run = false;
					this->cs.unlock();
					this->condition.notify_one();
					this->thread.join();

				}


				// Assign a task to this thread and wake it up.
				void Assign(std::shared_ptr<internal::TaskBase> task)
				{
					this->cs.lock();
						this->task = task;
					this->cs.unlock();
					this->condition.notify_one();
				}
			};


			// Maximum queue size
			static const int QUEUE_MAX = 1024;

			// Threading members
			std::mutex cs;
			std::thread thread;
			std::condition_variable condition;
			bool run = true;

			// The pool of threads
			std::array<Thread, N> threads;

			// The queue of tasks to be executed
			std::queue<std::shared_ptr<internal::TaskBase>> queue;
	};
}
