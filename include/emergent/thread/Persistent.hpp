#pragma once

#include <mutex>
#include <thread>
#include <future>


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

			inline void Run()
			{
				this->promise.set_value(this->job());
			}
		};

		template <> inline void Task<void>::Run()
		{
			this->job();
			this->promise.set_value();
		}
	}


	// A thread that does not exit when it has finished executing but waits
	// until it is assigned another job. An alternative to using std::async() where
	// a new thread is created each time (under gcc/clang).
	class PersistentThread
	{
		public:

			PersistentThread()
			{
				using namespace std::chrono;

				this->thread = std::thread([&] {
					std::unique_lock<std::mutex> lock(this->cs);

					this->run	= true;
					this->ready	= true;

					while (this->run)
					{
						if (this->task)
						{
							this->task->Run();
							this->task = nullptr;
							this->ready = true;
						}

						this->condition.wait(lock);
					}
				});

				// Wait until the thread has control of the mutex
				while (!this->run)
				{
					std::this_thread::sleep_for(microseconds(1));
				}
			}


			~PersistentThread()
			{
				// Allow the current task to complete before stopping
				this->cs.lock();
					this->run = false;
				this->cs.unlock();

				this->condition.notify_one();
				this->thread.join();
			}


			// Returns an invalid future if unable to assign the task.
			// This function will block if the thread is already busy but it is possible
			// to call it again before the condition has woken up and therefore it will not
			// assign the second task.
			template <typename T> auto Run(T &&job) -> std::future<decltype(job())>
			{
				std::lock_guard<std::mutex> lock(this->cs);

				if (this->Ready())
				{
					this->task	= std::make_shared<internal::Task<decltype(job())>>(std::move(job));
					this->ready	= false;
					this->condition.notify_one();

					return std::static_pointer_cast<internal::Task<decltype(job())>>(this->task)->promise.get_future();
				}

				return {};
			}


			bool Ready()
			{
				return this->run && this->ready;
			}


			// Directly assign a task to the thread and wake it up. This can be used
			// by a thread pool to allocate tasks but the Run function is the
			// recommended method in other situations.
			void Assign(std::shared_ptr<internal::TaskBase> task)
			{
				std::lock_guard<std::mutex> lock(this->cs);

				this->task	= task;
				this->ready	= false;
				this->condition.notify_one();
			}



		private:

			std::mutex cs;
			std::thread thread;
			std::condition_variable condition;
			std::shared_ptr<internal::TaskBase> task;
			std::atomic<bool> run	= false;
			std::atomic<bool> ready = false;

	};
}
