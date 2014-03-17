#pragma once

#include <ctime>
#include <mutex>
#include <vector>
#include <memory>
#include <fstream>
//#include <hiredis/hiredis.h>
#include <emergent/tinyformat.h>

#ifdef __linux__
#include <syslog.h>
#endif


namespace emergent
{

	/// Log a formatted message.
	#define FLOG(v, message, ...) LOG(v, tfm::format(message, __VA_ARGS__))

	/// Log a message.
	#define LOG(v, message) { if (v > logger::instance().verbosity); else logger::instance().log(v, __PRETTY_FUNCTION__, message); }


	const std::string NOW();	///< Produce a string timestamp
	const std::string FNOW();	///< Produce a string timestamp suitable for filenames
	const std::string DATE();	///< Produce a string of todays date


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
				void log(severity severity, std::string message);

				virtual ~sink() {}


			protected:
				/// Helper function to return a string representation of the severity level
				static const char *get_severity(severity severity);

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
				virtual void write(severity severity, std::string message);
		};



		/// Sink implementation for logging to a file (always appends).
		class file : public sink
		{
			public:
				file(std::string path);
				~file();

			protected:
				virtual void write(severity severity, std::string message);

			private:
				std::ofstream output;
		};


	#ifdef __linux__
		/// Sink implementation for logging to the syslog (available on a linux system)
		class syslog : public sink
		{
			protected:
				virtual void write(severity severity, std::string message);
		};
	#endif


		// Sink implementation for logging to redis pubsub
		/*class redis : public sink
		{
			public:
				redis(string channel, string server = "127.0.0.1", int port = 6379);
				~redis();
				
			protected:
				virtual void write(severity severity, string message);
				
			private:
				redis(const redis &r) = delete;				// Disable copy construction and assignment
				redis &operator=(const redis &r) = delete;	// operator due to it owning a redisContext.
				
				string channel;
				redisContext *context;
		};*/
	}


	/// The logging singleton.
	///
	/// A single instance of the logger is available within the process and may be used by
	/// concurrent threads. It has support for multiple sinks each of which will be used to
	/// log a message. It is advisable to use the LOG and FLOG macros for logging instead of
	/// accessing the log() function directly.
	class logger
	{
		public:
			/// Retrieves the singleton instance
			static logger& instance();

			/// Add a sink to the logger (takes ownership). Each message will be logged to all sinks.
			void add(sink::sink *sink);

			/// Log a message to each of the available sinks
			void log(severity severity, std::string trace, std::string message);

			/// Helper function to allow the verbosity to be set by string name.
			/// Useful for setting the verbosity via command-line argument.
			void set_verbosity(std::string verbosity = "");

			/// Get the current verbosity as a string or return all available verbosities.
			const char *get_verbosity(bool all = false);

			/// Indicate whether or not the origin of the call to LOG/FLOG (class and function)
			/// should be included in the log entry.
			bool trace = true;

			/// The current verbosity setting
			severity verbosity = warning;

		private:
			logger(); 							///< Prevent instantiation
			logger(logger const&);				///< Prevent copy construction
			logger& operator=(logger const&);	///< Prevent assignment
			~logger();

			/// List of available sinks
			std::vector<std::unique_ptr<sink::sink>> sinks;
	};
}