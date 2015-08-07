#pragma once

#include <string>
#include <mutex>
#include <fstream>
#include <iostream>
//#include <hiredis/hiredis.h>
#include <emergent/logger/Timestamp.hpp>

#ifdef __linux
	#include <syslog.h>
#endif


namespace emergent
{
	/// Available severity levels. These match some of those used by syslog.
	enum severity
	{
		fatal, error, warning, notice, info, debug
	};


	namespace sink
	{

		/// Abstract sink class from which all sink implementations derive.
		/// Each sink instance owns a critical section mutex and therefore
		/// no single sink can be used concurrently, but different sinks can.
		class sink
		{
			public:
				/// The main log function for the sink, it handles thread safety
				/// so that the sink implementations do not have to.
				void log(severity severity, std::string message)
				{
					std::lock_guard<std::mutex> lk(this->cs);
					this->write(severity, message);
				}

				virtual ~sink() {}


			protected:
				/// Helper function to return a string representation of the severity level
				static const char *get_severity(severity severity)
				{
					static const char *severities[] = { "FATAL", "ERROR", "WARNING", "NOTICE", "INFO", "DEBUG" };
					return severities[severity];
				}

				/// The function that actually does the writing and needs to be implemented
				/// by the concrete sinks
				virtual void write(severity severity, std::string message) = 0;

				/// Critical section mutex used to ensure that more than one process
				/// cannot log with the same sink simultaneously
				mutable std::mutex cs;
		};



		/// Sink implementation for logging to the console (stdout)
		class console : public sink
		{
			protected:
				virtual void write(severity severity, std::string message)
				{
					std::cout << this->get_severity(severity) << ": " << message << std::endl;
					std::flush(std::cout);
				}
		};



		/// Sink implementation for logging to a file (always appends).
		class file : public sink
		{
			public:

				file(std::string path)
				{
					this->output.exceptions(std::ofstream::failbit | std::ofstream::badbit);

					try
					{
						this->output.open(path, std::ios::out | std::ios::app);
					}
					catch (std::ofstream::failure e)
					{
						std::cout << "Exception opening log file: " << path << std::endl;
					}
				}

				~file()
				{
					if (this->output.is_open()) this->output.close();
				}

			protected:

				virtual void write(severity severity, std::string message)
				{

					if (this->output.is_open())
					{
						this->output << Timestamp::Now() << " " << this->get_severity(severity) << ": " << message << std::endl;
					}
				}

			private:
				std::ofstream output;
		};


	#ifdef __linux__
		/// Sink implementation for logging to the syslog (available on a linux system)
		class syslog : public sink
		{
			protected:

				virtual void write(severity severity, std::string message)
				{
					::syslog(LOG_CRIT + severity, "<%s> %s", this->get_severity(severity), message.c_str());
				}
		};
	#endif


		// Sink implementation for logging to redis pubsub
		/*class redis : public sink
		{
			public:

				redis(std::string channel, std::string server = "127.0.0.1", int port = 6379)
				{
					this->context	= redisConnect(server.c_str(), port);
					this->channel	= channel;

					if (this->context->err)	throw "Logging system failed to connect to redis";
				}

				~redis()
				{
					redisFree(this->context);
				}

			protected:

				virtual void write(severity severity, std::string message)
				{
					redisCommand(this->context, "PUBLISH %s %s:%s", this->channel.c_str(), this->get_severity(severity), message.c_str());
				}

			private:
				redis(const redis &r) = delete;				// Disable copy construction and assignment
				redis &operator=(const redis &r) = delete;	// operator due to it owning a redisContext.

				std::string channel;
				redisContext *context;
		};*/
	}
}
