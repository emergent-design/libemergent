#pragma once

#include <emergent/Logger.h>
#include <hiredis/adapters/libev.h>
#include <hiredis/async.h>
#include <algorithm>
#include <string.h>
#include <ev.h>
#include <set>


namespace emergent {
namespace redis
{
	using std::string;
	typedef std::function<void(string channel, string message)> broadcast;


	class Subscription
	{
		public:

			~Subscription()
			{
				if (this->context)	redisAsyncFree(this->context);
				if (this->loop)		ev_loop_destroy(this->loop);
			}


			bool Initialise(broadcast onMessage, std::set<string> channels = {}, bool socket = false, string connection = "127.0.0.1", int port = 6379)
			{
				this->port			= port;
				this->socket		= socket;
				this->connection	= connection;
				this->onMessage		= onMessage;
				this->channels		= channels;
				this->loop			= this->loop ? this->loop : ev_loop_new(EVFLAG_AUTO);

				return onMessage ? this->Connect() : false;
			}


			// Subscription not threaded. Drive event loop...
			void Listen()
			{
				if (this->context) ev_run(this->loop, EVRUN_NOWAIT);
			}


			bool Add(string channel)
			{
				this->channels.insert(channel);

				return this->context ? redisAsyncCommand(this->context, &Subscription::OnMessage, nullptr, "SUBSCRIBE %s", channel.c_str()) == REDIS_OK : true;
			}


			bool Remove(string channel)
			{
				this->channels.erase(channel);

				return this->context ? redisAsyncCommand(this->context, nullptr, nullptr, "UNSUBSCRIBE %s", channel.c_str()) == REDIS_OK : true;
			}


		private:

			bool Connect()
			{
				if (this->context)
				{
					if (this->context->err) FLOG(error, "Problem with redis async connection: %s", this->context->errstr);
					this->context	= nullptr;
				}

				this->context = this->socket ? redisAsyncConnectUnix(this->connection.c_str()) : redisAsyncConnect(this->connection.c_str(), this->port);

				if (this->context->err)
				{
					FLOG(error, "Failed to connect to redis async: %s", this->context->errstr);
					redisAsyncFree(this->context);
					this->context = nullptr;

					return false;
				}

				this->context->data = this;

				redisLibevAttach(this->loop, this->context);

				bool result = 	redisAsyncSetConnectCallback(this->context, &Subscription::OnConnect) == REDIS_OK
							&&	redisAsyncSetDisconnectCallback(this->context, &Subscription::OnDisconnect) == REDIS_OK;

				if (!result)
				{
					FLOG(error, "Failed to attach connection handlers to redis async: %s", this->context->errstr);
					redisAsyncFree(this->context);
					this->context = nullptr;
				}

				return result;
			}


			static void OnMessage(redisAsyncContext *c, void *reply, void *data)
			{
				if (reply)
				{
					redisReply *r = (redisReply *)reply;

					if (r->type == REDIS_REPLY_ARRAY && r->elements == 3)
					{
						if (strcmp(r->element[0]->str, "message") == 0)
						{
							((Subscription *)c->data)->onMessage(r->element[1]->str, std::string(r->element[2]->str, r->element[2]->len));
						}
					}
				}
			}


			static void OnConnect(const redisAsyncContext *context, int status)
			{
				for (auto &c : ((Subscription *)context->data)->channels)
				{
					redisAsyncCommand(const_cast<redisAsyncContext*>(context), &Subscription::OnMessage, nullptr, "SUBSCRIBE %s", c.c_str());
				}
			}


			static void OnDisconnect(const redisAsyncContext *context, int status)
			{
				if (status == REDIS_ERR) ((Subscription *)context->data)->Connect();
			}


			int port					= -1;
			bool socket					= false;
			string connection			= "127.0.0.1";
			broadcast onMessage			= nullptr;
			redisAsyncContext *context	= nullptr;
			struct ev_loop *loop		= nullptr;
			std::set<string> channels;
	};
}}
