#pragma once

#include <emergent/logger/Logger.hpp>
#include <hiredis/adapters/libev.h>
#include <hiredis/async.h>
#include <functional>
#include <string.h>
#include <ev.h>
#include <set>


namespace emergent {
namespace redis
{
	// #ifdef __cpp_lib_string_view
	// 	#include <string_view>
	// 	typedef std::function<void(std::string_view channel, std::string_view)> broadcast;
	// #else
	// 	typedef std::function<void(string channel, string message)> broadcast;
	// #endif

	typedef std::function<void(const string &channel, const string &message)> broadcast;

	using std::string;


	struct Channel
	{
		string name;
		bool pattern;

		Channel() {}
		Channel(string name)				: name(name), pattern(false) {}
		Channel(string name, bool pattern)	: name(name), pattern(pattern) {}
	};

	static bool operator <(const Channel &a, const Channel &b) { return a.name < b.name; }


	class Subscription
	{
		public:

			~Subscription()
			{
				if (this->context)	redisAsyncFree(this->context);
				if (this->loop)		ev_loop_destroy(this->loop);
			}


			bool Initialise(broadcast onMessage, std::set<Channel> channels = {}, bool socket = false, string connection = "127.0.0.1", int port = 6379)
			{
				this->port			= port;
				this->socket		= socket;
				this->connection	= connection;
				this->onMessage		= onMessage;
				this->channels		= channels;
				this->loop			= this->loop ? this->loop : ev_loop_new(EVFLAG_AUTO);

				return onMessage ? this->Connect() : false;
			}


			bool Connected() const
			{
				return this->connected;
			}


			// Subscription not threaded. Drive event loop...
			void Listen()
			{
				if (this->context)
				{
					ev_run(this->loop, EVRUN_NOWAIT);
				}
				else
				{
					this->Connect();
				}
			}


			bool Add(string channel, bool pattern = false)
			{
				this->channels.insert({ channel, pattern });

				return this->context
					? redisAsyncCommand(this->context, &Subscription::OnMessage, nullptr, pattern ? "PSUBSCRIBE %s" : "SUBSCRIBE %s", channel.c_str()) == REDIS_OK
					: true;
			}


			bool Remove(string channel, bool pattern = false)
			{
				this->channels.erase({ channel, pattern });

				return this->context
					? redisAsyncCommand(this->context, nullptr, nullptr, pattern ? "PUNSUBSCRIBE %s" : "UNSUBSCRIBE %s", channel.c_str()) == REDIS_OK
					: true;
			}


		private:

			bool Connect()
			{
				// Limit the time between reconnection attempts
				if (duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - this->lastAttempt).count() < CONNECT_TIMEOUT)
				{
					return false;
				}

				if (this->context)
				{
					// potentially already attempting a connection
					return false;
				}

				this->lastAttempt	= std::chrono::steady_clock::now();
				this->context		= this->socket
					? redisAsyncConnectUnix(this->connection.c_str())
					: redisAsyncConnect(this->connection.c_str(), this->port);

				if (this->context->err)
				{
					Log::Error("Failed to connect to redis async: %s", this->context->errstr);
					redisAsyncFree(this->context);
					this->context = nullptr;

					return false;
				}

				this->context->data = this;

				redisLibevAttach(this->loop, this->context);

				bool result = redisAsyncSetConnectCallback(this->context, &Subscription::OnConnect) == REDIS_OK
					&& redisAsyncSetDisconnectCallback(this->context, &Subscription::OnDisconnect) == REDIS_OK;

				if (!result)
				{
					Log::Error("Failed to attach connection handlers to redis async: %s", this->context->errstr);

					redisAsyncFree(this->context);
					this->context = nullptr;

					return false;
				}

				return true;
			}

			void Disconnect()
			{
				this->connected = false;

				if (this->context)
				{
					if (this->context->err)
					{
						Log::Error("Problem with redis async connection: %s", this->context->errstr);
					}

					// redisAsyncFree(this->context);
					this->context = nullptr;
				}
			}


			static void OnMessage(redisAsyncContext *c, void *reply, void *)
			{
				if (reply)
				{
					redisReply *r = (redisReply *)reply;

					if (r->type == REDIS_REPLY_ARRAY)
					{
						if (r->elements == 3 && strcmp(r->element[0]->str, "message") == 0)
						{
							// #ifdef __cpp_lib_string_view
							// 	((Subscription *)c->data)->onMessage(r->element[1]->str, std::string_view(r->element[2]->str, r->element[2]->len));
							// #else
								((Subscription *)c->data)->onMessage(r->element[1]->str, std::string(r->element[2]->str, r->element[2]->len));
							// #endif
						}
						else if (r->elements == 4 && strcmp(r->element[0]->str, "pmessage") == 0)
						{
							// #ifdef __cpp_lib_string_view
							// 	((Subscription *)c->data)->onMessage(r->element[2]->str, std::string_view(r->element[3]->str, r->element[3]->len));
							// #else
								((Subscription *)c->data)->onMessage(r->element[2]->str, std::string(r->element[3]->str, r->element[3]->len));
							// #endif
						}
					}
				}
			}


			static void OnConnect(const redisAsyncContext *context, int status)
			{
				((Subscription *)context->data)->connected = status == REDIS_OK;

				if (status != REDIS_OK)
				{
					((Subscription *)context->data)->Disconnect();
					return;
				}

				for (auto &c : ((Subscription *)context->data)->channels)
				{
					redisAsyncCommand(
						const_cast<redisAsyncContext*>(context),
						&Subscription::OnMessage,
						nullptr,
						c.pattern ? "PSUBSCRIBE %s" : "SUBSCRIBE %s",
						c.name.c_str()
					);
				}
			}


			static void OnDisconnect(const redisAsyncContext *context, int)
			{
				((Subscription *)context->data)->Disconnect();
			}


			static constexpr int CONNECT_TIMEOUT = 1000;	// Time (ms) between connection attempts

			int port					= -1;
			bool socket					= false;
			string connection			= "127.0.0.1";
			broadcast onMessage			= nullptr;
			redisAsyncContext *context	= nullptr;
			struct ev_loop *loop		= nullptr;
			bool connected				= false;

			std::chrono::time_point<std::chrono::steady_clock> lastAttempt;
			std::set<Channel> channels;

	};
}}
