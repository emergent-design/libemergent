#include "emergent/Logger.h"
#include <map>

using namespace std;


namespace emergent
{
	inline tm TIME()
	{
		tm result;
		time_t now = time(0);

		localtime_r(&now, &result);

		return result;
	}


	const string NOW()	{ tm t = TIME(); return tfm::format("%04d-%02d-%02d %02d:%02d:%02d",	1900 + t.tm_year, 1 + t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec); }
	const string FNOW()	{ tm t = TIME(); return tfm::format("%04d-%02d-%02d_%02d%02d%02d",		1900 + t.tm_year, 1 + t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec); }
	const string DATE()	{ tm t = TIME(); return tfm::format("%04d-%02d-%02d",					1900 + t.tm_year, 1 + t.tm_mon, t.tm_mday); }


	namespace sink
	{
		void sink::log(severity severity, string message)
		{
			lock_guard<mutex> lk(this->cs);
			this->write(severity, message);
		}


		const char *sink::get_severity(severity severity)
		{
			static const char *severities[] = { "FATAL", "ERROR", "WARNING", "NOTICE", "INFO", "DEBUG" };
			return severities[severity];
		}



		void console::write(severity severity, string message)
		{
			cout << this->get_severity(severity) << ": " << message << endl;
			flush(cout);
		}



		file::file(string path)
		{
			this->output.exceptions(ofstream::failbit | ofstream::badbit);

			try							{ this->output.open(path, ios::out | ios::app); }
			catch (ofstream::failure e)	{ cout << "Exception opening log file: " << path << endl; }
		}


		file::~file()
		{
			if (this->output.is_open()) this->output.close();
		}


		void file::write(severity severity, string message)
		{
			if (this->output.is_open()) this->output << NOW() << " " << this->get_severity(severity) << ": " << message << endl;
		}



	#ifdef __linux__
		void syslog::write(severity severity, string message)
		{
			::syslog(LOG_CRIT + severity, "<%s> %s", this->get_severity(severity), message.c_str());
		}
	#endif



		/*redis::redis(string channel, string server, int port)
		{
			this->context	= redisConnect(server.c_str(), port);
			this->channel	= channel;

			if (this->context->err)	throw "Logging system failed to connect to redis";
		}

		redis::~redis()
		{
			redisFree(this->context);
		}


		void redis::write(severity severity, string message)
		{
			redisCommand(this->context, "PUBLISH %s %s:%s", this->channel.c_str(), this->get_severity(severity), message.c_str());
		}*/
	}



	logger& logger::instance()
	{
		static logger _instance;
		return _instance;
	}


	void logger::add(sink::sink *sink)
	{
		this->sinks.emplace_back(sink);
	}


	void logger::log(severity severity, string trace, string message)
	{
		for (auto &sink : this->sinks) this->trace
			? sink->log(severity, message + " from '" + trace + "'")
			: sink->log(severity, message);
	}


	void logger::set_verbosity(string verbosity)
	{
		static const map<string, severity> m = {{"fatal", fatal}, {"error", error}, {"warning", warning}, {"notice", notice}, {"info", info}, {"debug", debug}};

		auto v = m.find(verbosity);
		if (v != m.end()) this->verbosity = v->second;
	}


	const char *logger::get_verbosity(bool all)
	{
		static const char *verbosities[] = { "fatal", "error", "warning", "notice", "info", "debug" };

		return all ? "debug, info, [warning], notice, error, fatal" : verbosities[this->verbosity];
	}


	logger::logger() 				{}
	logger::logger(logger const&)	{}
	logger::~logger()				{}

	logger& logger::operator=(logger const&)
	{
		return *this;
	}
}


