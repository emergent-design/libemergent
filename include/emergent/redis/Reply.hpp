#pragma once

// #include <emergent/Emergent.h>
#include <emergent/Logger.h>
#include <hiredis/hiredis.h>


namespace emergent {
namespace redis
{
	using std::string;
	using std::vector;

	class Reply
	{
		public:

			// Constructor takes ownership of the reply. It will free the reply object
			// and any associated elements (if it is an array type) when it goes out
			// of scope.
			Reply(void *reply)
			{
				if (reply)
				{
					this->reply = { (redisReply *)reply, [](redisReply *r) { freeReplyObject(r); }};

					if (this->reply->type == REDIS_REPLY_ARRAY && this->reply->element)
					{
						for (int i=0; i<this->reply->elements; i++)
						{
							this->elements.emplace_back(this->reply->element[i]);
						}
					}
				}
			}


			// Checks that this was initialised with a valid reply structure
			bool Ok()
			{
				return (bool)this->reply;
			}


			bool AsStatus()
			{
				return this->reply && this->reply->type == REDIS_REPLY_STATUS;
			}


			// Expects an integer type where 0 is false and >0 is true
			bool AsBool()
			{
				return this->reply && this->reply->type == REDIS_REPLY_INTEGER && this->reply->integer > 0;
			}


			long AsLong(long defaultValue = -1)
			{
				if (this->reply)
				{
					if (this->reply->type == REDIS_REPLY_STRING)
					{
						char *end	= nullptr;
						long value	= strtol(reply->str, &end, 10);

						return (end > reply->str) ? value : defaultValue;
					}
					else if (this->reply->type == REDIS_REPLY_INTEGER) return this->reply->integer;
				}

				return defaultValue;
			}


			double AsDouble(double defaultValue = 0.0)
			{
				if (this->reply && this->reply->type == REDIS_REPLY_STRING)
				{
					char *end		= nullptr;
					double value	= strtod(reply->str, &end);

					return (end > reply->str) ? value : defaultValue;
				}

				return defaultValue;
			}


			string AsString(string defaultValue = "")
			{
				return this->reply && this->reply->type == REDIS_REPLY_STRING ? this->reply->str : defaultValue;
			}


			vector<Reply> AsArray()
			{
				return this->elements;
			}


			vector<string> AsStringArray()
			{
				vector<string> result;

				for (auto &e : this->elements) result.push_back(e.AsString());

				return result;
			}


		private:

			std::shared_ptr<redisReply> reply;
			std::vector<Reply> elements;
	};

}}
