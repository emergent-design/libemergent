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
	enum class Severity
	{
		Fatal, Error, Warning, Notice, Info, Debug
	};


	namespace logger
	{
		// The abstract base class for a logging output. A derived class must
		// implemented the Write function. Thread safety is provided by the logger
		// itself which means it does not need be considered at the sink level.
		class Sink
		{
			public:

				virtual void Write(Severity severity, const std::string &message) = 0;
				virtual ~Sink() {}

			protected:

				static const char *ToString(Severity severity)
				{
					static const char *severities[] = { "<fatal>", "<error>", "<warning>", "<notice>", "<info>", "<debug>" };
					return severities[(int)severity];
				}
		};


		// A sink that outputs to the console (std::cout).
		class Console : public Sink
		{
			public:

				virtual void Write(Severity severity, const std::string &message)
				{
					std::cout << ToString(severity) << " " << message << std::endl;
					std::flush(std::cout);
				}
		};


	#ifdef __linux__
		/// Sink implementation for logging to the syslog (available on a linux system)
		class Syslog : public Sink
		{
			public:

				virtual void Write(Severity severity, const std::string &message)
				{
					::syslog(LOG_CRIT + (int)severity, "%s %s", ToString(severity), message.c_str());
				}
		};
	#endif


		// A sink for writing logs to a file. The calling application is responsible
		// for ensuring that the log file can be written to the path provided and for
		// any log rotation that may be required.
		class LogFile : public Sink
		{
			public:

				LogFile(std::string path)
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

				virtual void Write(Severity severity, const std::string &message)
				{
					if (this->output.is_open())
					{
						this->output << Timestamp::Now() << " " << ToString(severity) << " " << message << std::endl;
					}
				}

			private:

				std::ofstream output;
		};
	}

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
	// }
}
