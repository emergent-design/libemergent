#pragma once

#include <map>
#include <vector>
#include <memory>
#include <emergent/tinyformat.h>
#include <emergent/logger/Sinks.hpp>


namespace emergent
{
	/// Log a formatted message.
	#define FLOG(v, message, ...) LOG(v, tfm::format(message, __VA_ARGS__))

	/// Log a message.
	#ifdef __GNUC__
		#define LOG(v, message) { if (v > logger::instance().verbosity); else logger::instance().log(v, __PRETTY_FUNCTION__, message); }
	#else
		#define LOG(v, message) { if (v > logger::instance().verbosity); else logger::instance().log(v, __func__, message); }
	#endif


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
			inline static logger& instance()
			{
				static logger _instance;
				return _instance;
			}


			/// Add a sink to the logger (takes ownership). Each message will be logged to all sinks.
			void add(sink::sink *sink)
			{
				this->sinks.emplace_back(sink);
			}


			/// Log a message to each of the available sinks
			void log(severity severity, std::string trace, std::string message)
			{
				for (auto &sink : this->sinks) this->trace
					? sink->log(severity, message + " from '" + trace + "'")
					: sink->log(severity, message);
			}


			/// Helper function to allow the verbosity to be set by string name.
			/// Useful for setting the verbosity via command-line argument.
			void set_verbosity(std::string verbosity = "")
			{
				static const std::map<std::string, severity> m = {
					{"fatal", fatal}, {"error", error}, {"warning", warning}, {"notice", notice}, {"info", info}, {"debug", debug}
				};

				auto v = m.find(verbosity);
				if (v != m.end()) this->verbosity = v->second;
			}


			/// Get the current verbosity as a string or return all available verbosities.
			const char *get_verbosity(bool all = false)
			{
				static const char *verbosities[] = { "fatal", "error", "warning", "notice", "info", "debug" };

				return all ? "debug, info, [warning], notice, error, fatal" : verbosities[this->verbosity];
			}


			/// Indicate whether or not the origin of the call to LOG/FLOG (class and function)
			/// should be included in the log entry.
			bool trace = true;

			/// The current verbosity setting
			severity verbosity = warning;

		private:

			logger() {} 										///< Prevent instantiation
			logger(logger const&) {}							///< Prevent copy construction
			logger& operator=(logger const&) { return *this; }	///< Prevent assignment
			~logger() {}

			/// List of available sinks
			std::vector<std::unique_ptr<sink::sink>> sinks;
	};
}
