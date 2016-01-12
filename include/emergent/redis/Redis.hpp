#pragma once

#include <emergent/redis/Reply.hpp>


namespace emergent {
namespace redis
{
	using namespace std::chrono;

	class Redis
	{
		public:

			virtual ~Redis()
			{
				if (this->context) redisFree(this->context);
			}


			virtual bool Initialise(bool socket = false, string connection = "127.0.0.1", int port = 6379)
			{
				this->port			= port;
				this->socket		= socket;
				this->connection	= connection;

				return this->Connect();
			}


			// Yes, this uses C-style ellipses, but you cannot have a virtual templated parameter
			// pack function (the multiplexer version must be able to override the InvokeCommandV
			// see below).
			Reply Command(const char *command, ...)
			{
				va_list arguments;
				va_start(arguments, command);
					auto result = this->InvokeCommandV(command, arguments);
				va_end(arguments);

				return result;
			}


			// Connection
			bool Authenticate(string password)	{ return Command("AUTH %s", password.c_str()).AsStatus(); }
			bool Select(int db)					{ return Command("SELECT %d", db).AsStatus(); }

			// Keys
			//long Delete(std::initializer_list<string> keys) { }
			bool Delete(string key)							{ return Command("DEL %s", key.c_str()).AsBool(); }
			bool Exists(string key)							{ return Command("EXISTS %s", key.c_str()).AsBool(); }
			bool Expire(string key, long timeout)			{ return Command("EXPIRE %s %ld", key.c_str(), timeout).AsBool(); }
			bool ExpireAt(string key, long timestamp)		{ return Command("EXPIREAT %s %ld", key.c_str(), timestamp).AsBool(); }
			vector<string> Keys(string pattern = "*") 		{ return Command("KEYS %s", pattern.c_str()).AsStringArray(); }
			bool Persist(string key)						{ return Command("PERSIST %s", key.c_str()).AsBool(); }
			bool Rename(string key, string newKey)			{ return Command("RENAME %s %s", key.c_str(), newKey.c_str()).AsStatus(); }
			string Type(string key)							{ return Command("TYPE %s", key.c_str()).AsString(); }

			// Strings
			long Append(string key, string value)			{ return Command("APPEND %s %s", key.c_str(), value.c_str()).AsLong(); }
			long Decrement(string key)						{ return Command("DECR %s", key.c_str()).AsLong(); }
			long Decrement(string key, long amount)			{ return Command("DECRBY %s %ld", key.c_str(), amount).AsLong(); }
			string Get(string key)							{ return Command("GET %s", key.c_str()).AsString(); }
			long Get(string key, long defaultValue)			{ return Command("GET %s", key.c_str()).AsLong(defaultValue); }
			double Get(string key, double defaultValue)		{ return Command("GET %s", key.c_str()).AsDouble(defaultValue); }
			long Increment(string key)						{ return Command("INCR %s", key.c_str()).AsLong(); }
			long Increment(string key, long amount)			{ return Command("INCRBY %s %ld", key.c_str(), amount).AsLong(); }
			double Increment(string key, double amount)		{ return Command("INCRBYFLOAT %s %g", key.c_str(), amount).AsDouble(); }
			bool Set(string key, string value)				{ return Command("SET %s %s", key.c_str(), value.c_str()).Ok(); }
			bool Set(string key, long value)				{ return Command("SET %s %ld", key.c_str(), value).Ok(); }
			bool Set(string key, double value)				{ return Command("SET %s %g", key.c_str(), value).Ok(); }
			bool Set(string key, string value, long expiry)	{ return Command("SET %s %s EX %ld", key.c_str(), value.c_str(), expiry).Ok(); }
			long Length(string key)							{ return Command("STRLEN %s", key.c_str()).AsLong(); }

			// Lists
			string ListIndex(string key, int index)						{ return Command("LINDEX %s %ld", key.c_str(), index).AsString(); }
			long ListLength(string key)									{ return Command("LLEN %s", key.c_str()).AsLong(); }
			string PopFront(string key)									{ return Command("LPOP %s", key.c_str()).AsString(); }
			long PushFront(string key, string value)					{ return Command("LPUSH %s %s", key.c_str(), value.c_str()).AsLong(); }
			vector<string> ListRange(string key, int start, int stop)	{ return Command("LRANGE %s %ld %ld", key.c_str(), start, stop).AsStringArray(); }
			long ListRemove(string key, int count, string value)		{ return Command("LREM %s %ld %s", key.c_str(), count, value.c_str()).AsLong(); }
			bool ListSet(string key, int index, string value)			{ return Command("LSET %s %ld %s", key.c_str(), index, value.c_str()).Ok(); }
			bool ListTrim(string key, int start, int stop)				{ return Command("LTRIM %s %ld %ld", key.c_str(), start, stop).Ok(); }
			string PopBack(string key)									{ return Command("RPOP %s", key.c_str()).AsString(); }
			string PopPush(string src, string dst)						{ return Command("RPOPLPUSH %s %s", src.c_str(), dst.c_str()).AsString(); }
			long PushBack(string key, string value)						{ return Command("RPUSH %s %s", key.c_str(), value.c_str()).AsLong(); }

			long ListInsert(string key, string pivot, string value, bool before = true)
			{
				return Command("LINSERT %s %s %s %s", key.c_str(), before ? "BEFORE" : "AFTER", pivot.c_str(), value.c_str()).AsLong();
			}

			// Sets
			bool SetAdd(string key, string member)				{ return Command("SADD %s %s", key.c_str(), member.c_str()).AsBool(); }
			long Cardinality(string key)						{ return Command("SCARD %s", key.c_str()).AsLong(); }
			vector<string> Difference(string a, string b)		{ return Command("SDIFF %s %s", a.c_str(), b.c_str()).AsStringArray(); }
			vector<string> Intersection(string a, string b)		{ return Command("SINTER %s %s", a.c_str(), b.c_str()).AsStringArray(); }
			bool IsMember(string key, string member)			{ return Command("SISMEMBER %s %s", key.c_str(), member.c_str()).AsBool(); }
			vector<string> Members(string key)					{ return Command("SMEMBERS %s", key.c_str()).AsStringArray(); }
			bool SetMove(string src, string dst, string member)	{ return Command("SMOVE %s %s %s", src.c_str(), dst.c_str(), member.c_str()).AsBool(); }
			bool SetRemove(string key, string member)			{ return Command("SREM %s %s", key.c_str(), member.c_str()).AsBool(); }
			vector<string> Union(string a, string b)			{ return Command("SUNION %s %s", a.c_str(), b.c_str()).AsStringArray(); }

			// Hashes
			bool HashDelete(string key, string field)						{ return Command("HDEL %s %s", key.c_str(), field.c_str()).AsBool(); }
			bool HashExists(string key, string field)						{ return Command("HEXISTS %s %s", key.c_str(), field.c_str()).AsBool(); }
			string HashGet(string key, string field)						{ return Command("HGET %s %s", key.c_str(), field.c_str()).AsString(); }
			long HashGet(string key, string field, long defaultValue)		{ return Command("HGET %s %s", key.c_str(), field.c_str()).AsLong(defaultValue); }
			double HashGet(string key, string field, double defaultValue)	{ return Command("HGET %s %s", key.c_str(), field.c_str()).AsDouble(defaultValue); }
			map<string, string> HashGetAll(string key)						{ return Command("HGETALL %s", key.c_str()).AsStringMap(); }
			long HashIncrement(string key, string field, long amount)		{ return Command("HINCRBY %s %s %ld", key.c_str(), field.c_str(), amount).AsLong(); }
			double HashIncrement(string key, string field, double amount)	{ return Command("HINCRBYFLOAT %s %s %g", key.c_str(), field.c_str(), amount).AsDouble(); }
			vector<string> HashKeys(string key)								{ return Command("HKEYS %s", key.c_str()).AsStringArray(); }
			long HashLength(string key)										{ return Command("HLEN %s", key.c_str()).AsLong(); }
			bool HashSet(string key, string field, string value)			{ return Command("HSET %s %s %s", key.c_str(), field.c_str(), value.c_str()).AsBool(); }
			bool HashSet(string key, string field, long value)				{ return Command("HSET %s %s %ld", key.c_str(), field.c_str(), value).AsBool(); }
			bool HashSet(string key, string field, double value)			{ return Command("HSET %s %s %g", key.c_str(), field.c_str(), value).AsBool(); }
			long HashStringLength(string key, string field)					{ return Command("HSTRLEN %s %s", key.c_str(), field.c_str()).AsLong(); }
			vector<string> HashValues(string key)							{ return Command("HVALS %s", key.c_str()).AsStringArray(); }

			// Sorted Sets
			bool SortedAdd(string key, string member, double score = 0.0)						{ return Command("ZADD %s %g %s", key.c_str(), score, member.c_str()).AsBool(); }
			long SortedCardinality(string key)													{ return Command("ZCARD %s", key.c_str()).AsLong(); }
			long SortedCount(string key, double min, double max)								{ return Command("ZCOUNT %s %g %g", key.c_str(), min, max).AsLong(); }
			double SortedIncrement(string key, double increment, string member)					{ return Command("ZINCRBY %s %g %s", key.c_str(), increment, member.c_str()).AsDouble(); }
			long SortedLexCount(string key, string min = "-", string max = "+")					{ return Command("ZLEXCOUNT %s %s %s", key.c_str(), min.c_str(), max.c_str()).AsLong(); }
			vector<string> SortedRange(string key, long start, long stop)						{ return Command("ZRANGE %s %ld %ld", key.c_str(), start, stop).AsStringArray(); }
			vector<string> SortedRangeByLex(string key, string min = "-", string max = "+")		{ return Command("ZRANGEBYLEX %s %s %s", key.c_str(), min.c_str(), max.c_str()).AsStringArray(); }
			vector<string> SortedRangeByScore(string key, double min, double max)				{ return Command("ZRANGEBYSCORE %s %g %g", key.c_str(), min, max).AsStringArray(); }
			long SortedRank(string key, string member)											{ return Command("ZRANK %s %s", key.c_str(), member.c_str()).AsLong(); }
			bool SortedRemove(string key, string member)										{ return Command("ZREM %s %s", key.c_str(), member.c_str()).AsBool(); }
			long SortedRemoveByLex(string key, string min, string max)							{ return Command("ZREMRANGEBYLEX %s %s %s", key.c_str(), min.c_str(), max.c_str()).AsLong(); }
			long SortedRemoveByRank(string key, long start, long stop)							{ return Command("ZREMRANGEBYRANK %s %ld %ld", key.c_str(), start, stop).AsLong(); }
			long SortedRemoveByScore(string key, double min, double max)						{ return Command("ZREMRANGEBYSCORE %s %g %g", key.c_str(), min, max).AsLong(); }
			vector<string> SortedRevRange(string key, long start, long stop)					{ return Command("ZREVRANGE %s %ld %ld", key.c_str(), start, stop).AsStringArray(); }
			vector<string> SortedRevRangeByLex(string key, string max = "+", string min = "-")	{ return Command("ZREVRANGEBYLEX %s %s %s", key.c_str(), max.c_str(), min.c_str()).AsStringArray(); }
			vector<string> SortedRevRangeByScore(string key, double min, double max)			{ return Command("ZREVRANGEBYSCORE %s %g %g", key.c_str(), min, max).AsStringArray(); }
			long SortedRevRank(string key, string member)										{ return Command("ZREVRANK %s %s", key.c_str(), member.c_str()).AsLong(); }
			double SortedScore(string key, string member)										{ return Command("ZSCORE %s %s", key.c_str(), member.c_str()).AsDouble(); }
			// HLL?

			// Publish
			long Publish(string channel, string message)		{ return Command("PUBLISH %s %s", channel.c_str(), message.c_str()).AsLong(); }
			long Publish(string channel, long value)			{ return Command("PUBLISH %s %ld", channel.c_str(), value).AsLong(); }
			long Publish(string channel, double value)			{ return Command("PUBLISH %s %g", channel.c_str(), value).AsLong(); }
			long PublishBinary(string channel, string message)	{ return Command("PUBLISH %s %b", channel.c_str(), message.data(), (size_t)message.size()).AsLong(); }
			// Transactions?

		protected:


			redisReply *InvokeCommand(const char *command, ...)
			{
				va_list arguments;
				va_start(arguments, command);
					auto result = this->InvokeCommandV(command, arguments);
				va_end(arguments);

				return result;
			}


			virtual redisReply *InvokeCommandV(const char *command, va_list &arguments)
			{
				if (this->context)
				{
					auto result = redisvCommand(this->context, command, arguments);

					if (!result) this->Connect();

					return (redisReply *)result;
				}

				return nullptr;
			}


			bool Connect()
			{
				if (this->context)
				{
					if (this->context->err) Log::Error("Problem with redis connection: %s", this->context->errstr);
					redisFree(this->context);
					this->context = nullptr;
				}

				// Limit the time between reconnection attempts
				if (duration_cast<milliseconds>(steady_clock::now() - this->lastAttempt).count() < this->timeout)
				{
					return false;
				}

				this->context 		= this->socket ? redisConnectUnix(this->connection.c_str()) : redisConnect(this->connection.c_str(), this->port);
				this->lastAttempt	= steady_clock::now();

				if (this->context->err)
				{
					Log::Error("Failed to connect to redis: %s", this->context->errstr);
					redisFree(this->context);
					this->context = nullptr;

					return false;
				}

				return true;
			}


			int port								= -1;
			int timeout								= 1000;
			bool socket								= false;
			string connection						= "127.0.0.1";
			redisContext *context					= nullptr;
			time_point<steady_clock> lastAttempt;
	};
}}
