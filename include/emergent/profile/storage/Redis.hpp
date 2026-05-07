#pragma once

#include <emergent/profile/Storage.hpp>
#include <emergent/redis/Redis.hpp>
#include <entity/json.hpp>


namespace emergent
{
	namespace profile
	{
		/// Aggregated results will be stored to Redis. The results are serialised to JSON and
		/// then stored at the key "profile:ID" where ID is the with which the Aggregator singleton
		/// was initialised.
		class RedisStorage : public Storage
		{
			public:

				RedisStorage(bool socket = false, std::string connection = "127.0.0.1", int port = 6379)
					: port(port), socket(socket), connection(connection) {}


				bool Initialise() override
				{
					return this->cache.Initialise(socket, connection, port);
				}


				void Store(Aggregation &aggregation) override
				{
					this->cache.HashSet(
						"profiles", aggregation.id,
						ent::encode<ent::json>(aggregation)
					);
				}


			private:

				redis::Redis cache;

				int port;
				bool socket;
				std::string connection;
		};
	}
}
