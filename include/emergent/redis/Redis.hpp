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


			virtual bool Initialise(bool socket = false, const string &connection = "127.0.0.1", int port = 6379)
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
			bool Authenticate(const string &password)	{ return Command("AUTH %s", password.c_str()).AsStatus(); }
			bool Select(int db)							{ return Command("SELECT %d", db).AsStatus(); }

			// Keys
			//long Delete(std::initializer_list<string> keys) { }
			bool Delete(const string &key)							{ return Command("DEL %s", key.c_str()).AsBool(); }
			bool Exists(const string &key)							{ return Command("EXISTS %s", key.c_str()).AsBool(); }
			bool Expire(const string &key, long timeout)			{ return Command("EXPIRE %s %ld", key.c_str(), timeout).AsBool(); }
			bool ExpireAt(const string &key, long timestamp)		{ return Command("EXPIREAT %s %ld", key.c_str(), timestamp).AsBool(); }
			vector<string> Keys(const string &pattern = "*") 		{ return Command("KEYS %s", pattern.c_str()).AsStringArray(); }
			bool Persist(const string &key)							{ return Command("PERSIST %s", key.c_str()).AsBool(); }
			bool Rename(const string &key, const string &newKey)	{ return Command("RENAME %s %s", key.c_str(), newKey.c_str()).AsStatus(); }
			string Type(const string &key)							{ return Command("TYPE %s", key.c_str()).AsString(); }
			long TimeToLive(const string &key)						{ return Command("TTL %s", key.c_str()).AsLong(); }

			// Strings
			long Append(const string &key, const string &value)				{ return Command("APPEND %s %b", key.c_str(), value.data(), value.size()).AsLong(); }
			long Decrement(const string &key)								{ return Command("DECR %s", key.c_str()).AsLong(); }
			long Decrement(const string &key, long amount)					{ return Command("DECRBY %s %ld", key.c_str(), amount).AsLong(); }
			string Get(const string &key)									{ return Command("GET %s", key.c_str()).AsString(); }
			long Get(const string &key, long defaultValue)					{ return Command("GET %s", key.c_str()).AsLong(defaultValue); }
			double Get(const string &key, double defaultValue)				{ return Command("GET %s", key.c_str()).AsDouble(defaultValue); }
			long Increment(const string &key)								{ return Command("INCR %s", key.c_str()).AsLong(); }
			long Increment(const string &key, long amount)					{ return Command("INCRBY %s %ld", key.c_str(), amount).AsLong(); }
			double Increment(const string &key, double amount)				{ return Command("INCRBYFLOAT %s %g", key.c_str(), amount).AsDouble(); }
			bool Set(const string &key, const string &value)				{ return Command("SET %s %b", key.c_str(), value.data(), (size_t)value.size()).Ok(); }
			bool Set(const string &key, long value)							{ return Command("SET %s %ld", key.c_str(), value).Ok(); }
			bool Set(const string &key, double value)						{ return Command("SET %s %g", key.c_str(), value).Ok(); }
			bool Set(const string &key, const string &value, long expiry)	{ return Command("SET %s %b EX %ld", key.c_str(), value.data(), value.size(), expiry).Ok(); }
			long Length(const string &key)									{ return Command("STRLEN %s", key.c_str()).AsLong(); }
			pair<long, vector<string>> Scan(long cursor)					{ return Command("SCAN %ld", cursor).AsScanArray(); }

			// Lists
			string ListIndex(const string &key, int index)						{ return Command("LINDEX %s %ld", key.c_str(), index).AsString(); }
			long ListLength(const string &key)									{ return Command("LLEN %s", key.c_str()).AsLong(); }
			string PopFront(const string &key)									{ return Command("LPOP %s", key.c_str()).AsString(); }
			long PushFront(const string &key, const string &value)				{ return Command("LPUSH %s %b", key.c_str(), value.data(), (size_t)value.size()).AsLong(); }
			vector<string> ListRange (const string &key, int start, int stop)	{ return Command("LRANGE %s %ld %ld", key.c_str(), start, stop).AsStringArray(); }
			long ListRemove(const string &key, int count, const string &value)	{ return Command("LREM %s %ld %b", key.c_str(), count, value.data(), (size_t)value.size()).AsLong(); }
			bool ListSet(const string &key, int index, const string &value)		{ return Command("LSET %s %ld %b", key.c_str(), index, value.data(), (size_t)value.size()).Ok(); }
			bool ListTrim(const string &key, int start, int stop)				{ return Command("LTRIM %s %ld %ld", key.c_str(), start, stop).Ok(); }
			string PopBack(const string &key)									{ return Command("RPOP %s", key.c_str()).AsString(); }
			string PopPush(const string &src, const string &dst)				{ return Command("RPOPLPUSH %s %s", src.c_str(), dst.c_str()).AsString(); }
			long PushBack(const string &key, const string &value)				{ return Command("RPUSH %s %b", key.c_str(), value.data(), (size_t)value.size()).AsLong(); }

			long ListInsert(const string &key, const string &pivot, const string &value, bool before = true)
			{
				return Command("LINSERT %s %s %b %b", key.c_str(), before ? "BEFORE" : "AFTER", pivot.data(), (size_t)pivot.size(), value.data(), (size_t)value.size()).AsLong();
			}

			// Sets
			bool SetAdd(const string &key, const string &member)						{ return Command("SADD %s %b", key.c_str(), member.data(), (size_t)member.size()).AsBool(); }
			long Cardinality(const string &key)											{ return Command("SCARD %s", key.c_str()).AsLong(); }
			vector<string> Difference(const string &a, const string &b)					{ return Command("SDIFF %s %s", a.c_str(), b.c_str()).AsStringArray(); }
			vector<string> Intersection(const string &a, const string &b)				{ return Command("SINTER %s %s", a.c_str(), b.c_str()).AsStringArray(); }
			bool IsMember(const string &key, const string &member)						{ return Command("SISMEMBER %s %b", key.c_str(), member.data(), (size_t)member.size()).AsBool(); }
			vector<string> Members(const string &key)									{ return Command("SMEMBERS %s", key.c_str()).AsStringArray(); }
			bool SetMove(const string &src, const string &dst, const string &member)	{ return Command("SMOVE %s %s %b", src.c_str(), dst.c_str(), member.data(), (size_t)member.size()).AsBool(); }
			bool SetRemove(const string &key, const string &member)						{ return Command("SREM %s %b", key.c_str(), member.data(), (size_t)member.size()).AsBool(); }
			vector<string> Union(const string &a, const string &b)						{ return Command("SUNION %s %s", a.c_str(), b.c_str()).AsStringArray(); }
			pair<long, vector<string>> SetScan(const string &key, long cursor)			{ return Command("SSCAN %s %ld", key.c_str(), cursor).AsScanArray(); }

			// Hashes
			bool HashDelete(const string &key, const string &field)						{ return Command("HDEL %s %s", key.c_str(), field.c_str()).AsBool(); }
			bool HashExists(const string &key, const string &field)						{ return Command("HEXISTS %s %s", key.c_str(), field.c_str()).AsBool(); }
			string HashGet(const string &key, const string &field)						{ return Command("HGET %s %s", key.c_str(), field.c_str()).AsString(); }
			long HashGet(const string &key, const string &field, long defaultValue)		{ return Command("HGET %s %s", key.c_str(), field.c_str()).AsLong(defaultValue); }
			double HashGet(const string &key, const string &field, double defaultValue)	{ return Command("HGET %s %s", key.c_str(), field.c_str()).AsDouble(defaultValue); }
			map<string, string> HashGetAll(const string &key)							{ return Command("HGETALL %s", key.c_str()).AsStringMap(); }
			long HashIncrement(const string &key, const string &field, long amount)		{ return Command("HINCRBY %s %s %ld", key.c_str(), field.c_str(), amount).AsLong(); }
			double HashIncrement(const string &key, const string &field, double amount)	{ return Command("HINCRBYFLOAT %s %s %g", key.c_str(), field.c_str(), amount).AsDouble(); }
			vector<string> HashKeys(const string &key)									{ return Command("HKEYS %s", key.c_str()).AsStringArray(); }
			long HashLength(const string &key)											{ return Command("HLEN %s", key.c_str()).AsLong(); }
			bool HashSet(const string &key, const string &field, const string &value)	{ return Command("HSET %s %s %b", key.c_str(), field.c_str(), value.data(), (size_t)value.size()).AsBool(); }
			bool HashSet(const string &key, const string &field, long value)			{ return Command("HSET %s %s %ld", key.c_str(), field.c_str(), value).AsBool(); }
			bool HashSet(const string &key, const string &field, double value)			{ return Command("HSET %s %s %g", key.c_str(), field.c_str(), value).AsBool(); }
			long HashStringLength(const string &key, const string &field)				{ return Command("HSTRLEN %s %s", key.c_str(), field.c_str()).AsLong(); }
			vector<string> HashValues(const string &key)								{ return Command("HVALS %s", key.c_str()).AsStringArray(); }
			pair<long, map<string, string>> HashScan(const string &key, long cursor)	{ return Command("HSCAN %s %ld", key.c_str(), cursor).AsScanMap(); }

			// Sorted Sets
			bool SortedAdd(const string &key, const string &member, double score = 0.0)								{ return Command("ZADD %s %g %b", key.c_str(), score, member.data(), (size_t)member.size()).AsBool(); }
			long SortedCardinality(const string &key)																{ return Command("ZCARD %s", key.c_str()).AsLong(); }
			long SortedCount(const string &key, double min, double max)												{ return Command("ZCOUNT %s %g %g", key.c_str(), min, max).AsLong(); }
			double SortedIncrement(const string &key, double increment, const string &member)						{ return Command("ZINCRBY %s %g %b", key.c_str(), increment, member.data(), (size_t)member.size()).AsDouble(); }
			long SortedLexCount(const string &key, const string &min = "-", const string &max = "+")				{ return Command("ZLEXCOUNT %s %s %s", key.c_str(), min.c_str(), max.c_str()).AsLong(); }
			vector<string> SortedRange(const string &key, long start, long stop)									{ return Command("ZRANGE %s %ld %ld", key.c_str(), start, stop).AsStringArray(); }
			vector<string> SortedRangeByLex(const string &key, const string &min = "-", const string &max = "+")	{ return Command("ZRANGEBYLEX %s %s %s", key.c_str(), min.c_str(), max.c_str()).AsStringArray(); }
			vector<string> SortedRangeByScore(const string &key, double min, double max)							{ return Command("ZRANGEBYSCORE %s %g %g", key.c_str(), min, max).AsStringArray(); }
			long SortedRank(const string &key, const string &member)												{ return Command("ZRANK %s %b", key.c_str(), member.data(), (size_t)member.size()).AsLong(); }
			bool SortedRemove(const string &key, const string &member)												{ return Command("ZREM %s %b", key.c_str(), member.data(), (size_t)member.size()).AsBool(); }
			long SortedRemoveByLex(const string &key, const string &min, const string &max)							{ return Command("ZREMRANGEBYLEX %s %s %s", key.c_str(), min.c_str(), max.c_str()).AsLong(); }
			long SortedRemoveByRank(const string &key, long start, long stop)										{ return Command("ZREMRANGEBYRANK %s %ld %ld", key.c_str(), start, stop).AsLong(); }
			long SortedRemoveByScore(const string &key, double min, double max)										{ return Command("ZREMRANGEBYSCORE %s %g %g", key.c_str(), min, max).AsLong(); }
			vector<string> SortedRevRange(const string &key, long start, long stop)									{ return Command("ZREVRANGE %s %ld %ld", key.c_str(), start, stop).AsStringArray(); }
			vector<string> SortedRevRangeByLex(const string &key, const string &max = "+", const string &min = "-")	{ return Command("ZREVRANGEBYLEX %s %s %s", key.c_str(), max.c_str(), min.c_str()).AsStringArray(); }
			vector<string> SortedRevRangeByScore(const string &key, double min, double max)							{ return Command("ZREVRANGEBYSCORE %s %g %g", key.c_str(), min, max).AsStringArray(); }
			long SortedRevRank(const string &key, const string &member)												{ return Command("ZREVRANK %s %b", key.c_str(), member.data(), (size_t)member.size()).AsLong(); }
			double SortedScore(const string &key, const string &member)												{ return Command("ZSCORE %s %b", key.c_str(), member.data(), (size_t)member.size()).AsDouble(); }
			pair<long, map<string, string>> SortedScan(const string &key, long cursor)								{ return Command("ZSCAN %s %ld", key.c_str(), cursor).AsScanMap(); }
			// HLL?

			// Publish
			long Publish(const string &channel, const string &message)	{ return Command("PUBLISH %s %b", channel.c_str(), message.data(), (size_t)message.size()).AsLong(); }
			long Publish(const string &channel, long value)				{ return Command("PUBLISH %s %ld", channel.c_str(), value).AsLong(); }
			long Publish(const string &channel, double value)			{ return Command("PUBLISH %s %g", channel.c_str(), value).AsLong(); }
			// long PublishBinary(string channel, string message)	{ return Command("PUBLISH %s %b", channel.c_str(), message.data(), (size_t)message.size()).AsLong(); }
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
					Log::Error("Failed to connect to redis at %s: %s", this->connection, this->context->errstr);
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
