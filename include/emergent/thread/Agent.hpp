#pragma once

#include <thread>
#include <atomic>
#include <condition_variable>
#include <emergent/Type.hpp>


namespace emergent
{
	enum class AgentMode
	{
		Sleep,		// Simple sleep after each poll
		Interval,	// Sleep duration is calculated to achieve a specific polling interval
		Blocking,	// Blocks waiting for a conditional notification between polls
		Timeout		// Waits for a conditional notification or timeout between polls
	};


	// Provides a threaded agent which is responsible for invoking a subject.
	// This reverses the ownership hierarchy when using derived classes
	// of T so that the thread is always exited before the derivative of
	// T is destroyed (avoiding the issues that can occur when a base class
	// is responsible for the threading). The constant SLEEP defines the sleep
	// time for the thread in microseconds.
	//
	// A subject base class is expected to implement the following interface
	// {
	// 		public:
	//			void Initialise(...)
	// 			void OnEntry();
	// 			void OnExit();
	// 			void Poll();
	// }
	template <typename T> class Agent
	{
		public:


			Agent()
			{
				this->run = false;
			}


			~Agent()
			{
				if (this->run)
				{
					this->run 			= false;
					this->allowExecute	= false;
					this->condition.notify_one();
					this->thread.join();
				}
			}


			template <typename R, typename P> void Duration(const std::chrono::duration<R, P> &duration)
			{
				this->duration = duration;
			}


			// Attempts to construct a derivative subject of T given the type name. If
			// successful the additional arguments are passed to the initialisation
			// function of the subject and then the thread is started in the selected mode.
			template <typename R, typename P, class... Args> bool Initialise(AgentMode mode, const std::chrono::duration<R, P> &duration, const std::string &type, Args&&... args)
			{
				if (!this->run)
				{
					this->duration	= duration;
					this->subject	= emg::Type<T>::Create(type);

					// If subject initialisation fails then the thread is not started.
					if (this->subject && this->subject->Initialise(std::forward<Args>(args)...))
					{
						this->run = true;

						switch (mode)
						{
							case AgentMode::Sleep:		this->thread = std::thread(&Agent::Sleep, this);	break;
							case AgentMode::Interval:	this->thread = std::thread(&Agent::Interval, this);	break;
							case AgentMode::Blocking:	this->thread = std::thread(&Agent::Blocking, this);	break;
							case AgentMode::Timeout:	this->thread = std::thread(&Agent::Timeout, this);	break;
						}

						return true;
					}
				}

				return false;
			}


			// Thread-safe execution of function on the subject and guarantees
			// that the thread is not in the process of polling. If the action
			// returns true then the thread is awoken to perform the next poll.
			// It can only be used in Blocking and Timeout modes and will not
			// perform the action if this agent is in Sleep or Interval mode.
			void Execute(std::function<bool(T*)> action)
			{
				if (this->subject && this->allowExecute)
				{
					this->cs.lock();
						auto result = action(this->subject.get());
					this->cs.unlock();

					if (result)
					{
						this->condition.notify_one();
					}
				}
			}


			// UNSAFE: Allows access to the underlying subject directly, but this
			// is asynchronous to the agent thread. If a guarantee of thread safety
			// is required for an operation on the subject then use the Execute helper
			// function instead.
			T *Subject()
			{
				return this->subject.get();
			}


		private:

			void Sleep()
			{
				this->subject->OnEntry();

				while (this->run)
				{
					this->subject->Poll();
					std::this_thread::sleep_for(this->duration);
				}

				this->subject->OnExit();
			}


			void Interval()
			{
				std::chrono::time_point<std::chrono::steady_clock> wait;

				this->subject->OnEntry();

				while (this->run)
				{
					wait = std::chrono::steady_clock::now() + this->duration;

					this->subject->Poll();

					std::this_thread::sleep_until(wait);
				}

				this->subject->OnExit();
			}


			void Blocking()
			{
				std::unique_lock<std::mutex> lock(this->cs);

				this->allowExecute = true;
				this->subject->OnEntry();

				while (this->run)
				{
					this->subject->Poll();
					this->condition.wait(lock);
				}

				this->subject->OnExit();
			}


			void Timeout()
			{
				std::unique_lock<std::mutex> lock(this->cs);

				this->allowExecute = true;
				this->subject->OnEntry();

				while (this->run)
				{
					this->subject->Poll();
					this->condition.wait_for(lock, this->duration);
				}

				this->subject->OnExit();
			}


			bool allowExecute = false;
			std::chrono::microseconds duration { 1000 };
			std::unique_ptr<T> subject;
			std::condition_variable condition;
			std::atomic<bool> run;
			std::thread thread;
			std::mutex cs;
	};
}
