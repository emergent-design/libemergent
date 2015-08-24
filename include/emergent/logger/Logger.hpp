#pragma once

#include <map>
#include <vector>
#include <memory>
#include <emergent/concurrentqueue.h>
#include <emergent/logger/Sinks.hpp>
#include <emergent/String.hpp>


namespace emergent
{
	class Log
	{
		public:

			// Push a log message onto the queue with a custom severity. If the severity is less than that
			// required by the logging verbosity then it will not be queued.
			template <typename ...Args> static inline void Write(Severity severity, const char *message, Args ...args)
			{
				if (severity > Instance().verbosity)	return;
				if (!Instance().run) 					return;

				Instance().queue.enqueue({
					severity, sizeof...(args) ? String::format(message, args...) : message
				});
			}


			// Helper functions for logging under various severities
			template <typename ...Args> static inline void Debug(const char *message, Args ...args)		{ Write(Severity::Debug, message, args...); }
			template <typename ...Args> static inline void Info(const char *message, Args ...args)		{ Write(Severity::Info, message, args...); }
			template <typename ...Args> static inline void Warning(const char *message, Args ...args)	{ Write(Severity::Warning, message, args...); }
			template <typename ...Args> static inline void Error(const char *message, Args ...args)		{ Write(Severity::Error, message, args...); }


			// The logger cannot be used until it is initialised. The logger will write logs
			// to all sinks provided (available options can be found in the Sinks.hpp file
			// or custom sinks may be implemented). This function may only be called once and
			// will start the logging thread.
			static bool Initialise(std::initializer_list<std::unique_ptr<logger::Sink>> sinks)
			{
				if (!Instance().run)
				{
					for (auto &s : sinks)
					{
						// Eww, I know this is horrible but it is the only way I could find to
						// use an initializer_list with unique_ptr<>.
						Instance().sinks.push_back(
							std::move(*const_cast<std::unique_ptr<logger::Sink>*>(&s))
						);
					}

					Instance().run		= true;
					Instance().thread	= std::thread(&Log::Entry, &Instance());

					return true;
				}

				return false;
			}


			// Return the current verbosity setting
			static Severity Verbosity()
			{
				return Instance().verbosity;
			}


			// Set the logging verbosity
			static void Verbosity(Severity verbosity)
			{
				Instance().verbosity = verbosity;
			}


			// Set the logging verbosity by string name.
			static void Verbosity(const std::string &verbosity)
			{
				static std::map<std::string, Severity> m = {
					{ "fatal", Severity::Fatal },	{ "notice", Severity::Notice },
					{ "error", Severity::Error },	{ "warning", Severity::Warning },
					{ "info", Severity::Info },		{ "debug", Severity::Debug }
				};

				if (m.count(verbosity)) Instance().verbosity = m[verbosity];
			}


			// Approximate backlog count for the message queue.
			static long Backlog()
			{
				return Instance().queue.size_approx();
			}


		private:

			// Simple structure used for queueing logs
			struct Item
			{
				Severity severity;
				std::string message;
			};


			// Accessor for the singleton instance
			inline static Log &Instance()
			{
				static Log _instance;
				return _instance;
			}

			// Prevent instantiation
			Log()
			{
				this->run = false;
			}

			// Prevent assignment
			Log& operator=(Log const&)
			{
				return *this;
			}

			// Prevent copy construction
			Log(Log const&) {}


			// When destroyed stop the logging thread regardless of the
			// remaining queue size.
			~Log()
			{
				if (this->run)
				{
					this->run = false;
					this->thread.join();
				}
			}


			// Entry point for the logging thread responsible for pulling
			// items out of the queue and then writing them to all sinks.
			void Entry()
			{
				using namespace std::chrono;

				Item item;

				while (this->run)
				{
					// Only sleep if there is nothing in the queue
					if (this->queue.try_dequeue(item))
					{
						for (auto &sink : this->sinks)
						{
							sink->Write(item.severity, item.message);
						}
					}
					else std::this_thread::sleep_for(milliseconds(10));
				}
			}


			// The current verbosity setting
			Severity verbosity = Severity::Warning;

			// Sinks owned by the logger (provided during initialisation)
			std::vector<std::unique_ptr<logger::Sink>> sinks;

			// A thread-safe queue for storing messages before writing
			moodycamel::ConcurrentQueue<Item> queue;

			// Threading members
			std::atomic<bool> run;
			std::thread thread;
	};
}
