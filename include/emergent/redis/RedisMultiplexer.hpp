#pragma once

#include <queue>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <emergent/redis/Redis.hpp>


namespace emergent {
namespace redis
{
	// Single redis connection, thread-safe with automatic pipelining.
	// Since Command is blocking it only pipelines concurrent calls to the multiplexer.
	// This means that the size of the pipeline is a function of the number of threads
	// using the multiplexer but there are no limits placed on this for the following
	// reason: the redis documentation suggests that 10k commands is a reasonable size
	// for pipelining, yet running with 10k threads is probably a bad idea so it is
	// unlikely that an application will cause excessive pipelining.
	class RedisMultiplexer : public Redis
	{
		public:

			RedisMultiplexer()
			{
				this->run		= true;
				this->thread	= std::thread(&RedisMultiplexer::Entry, this);
			}


			virtual ~RedisMultiplexer()
			{
				this->run = false;
				this->condition.notify_one();
				this->thread.join();
			}


			virtual Reply Command(const char *command, ...)
			{
				int result			= REDIS_ERR;
				auto expectation	= std::make_shared<Expectation>();
				std::unique_lock<std::mutex> lock(expectation->cs);

				this->cs.lock();

					if (this->context)
					{
						va_list arguments;
						va_start(arguments, command);
							result = redisvAppendCommand(this->context, command, arguments);
						va_end(arguments);

						if (result == REDIS_OK)
						{
							this->expectations.push(expectation);
						}
					}

				this->cs.unlock();

				// Notify the multiplexer thread that there are pipelined
				// commands and expectations to handle.
				this->condition.notify_one();

				if (result == REDIS_OK)
				{
					// Timeout?
					expectation->condition.wait(lock);

					return expectation->reply;
				}

				return nullptr;
			}


		protected:

			struct Expectation
			{
				std::mutex cs;
				std::condition_variable condition;
				redisReply *reply = nullptr;


				void HandleReply(redisReply *reply)
				{
					this->cs.lock();
						this->reply = reply;
					this->cs.unlock();
					this->condition.notify_one();
				}
			};


			void Entry()
			{
				int result;
				redisReply *reply;
				std::unique_lock<std::mutex> lock(this->cs);

				bool connected = this->context;

				while (this->run)
				{
					if (connected)
					{
						while (!this->expectations.empty())
						{
							result = redisGetReply(this->context, (void **)&reply);

							this->expectations.front()->HandleReply(result == REDIS_OK ? reply : nullptr);
							this->expectations.pop();

							if (result != REDIS_OK) connected = false;
						}
					}

					if (!connected)
					{
						connected = this->Connect();
					}

					this->condition.wait(lock);
				}

				// Make sure we're not blocking any calling threads before exiting
				while (!this->expectations.empty())
				{
					this->expectations.front()->HandleReply(nullptr);
					this->expectations.pop();
				}
			}

			std::queue<std::shared_ptr<Expectation>> expectations;
			std::condition_variable condition;
			std::atomic<bool> run;
			std::thread thread;
			std::mutex cs;
	};
}}

