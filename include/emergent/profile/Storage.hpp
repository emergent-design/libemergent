#pragma once

#include <emergent/profile/Aggregation.hpp>
#include <emergent/redis/Redis.hpp>
#include <emergent/String.hpp>
#include <emergent/Path.hpp>
#include <entity/json.hpp>
#include <iostream>
#include <map>


namespace emergent
{
	namespace profile
	{
		/// Abstract base storage class. Derive from this to create
		/// custom storage types.
		class Storage
		{
			public:

				virtual bool Initialise() = 0;
				virtual void Store(Aggregation &aggregation) = 0;
				virtual void Close() {}
		};


		/// Simple storage which writes a pretty-printed JSON version
		/// of the aggregated results to the console.
		class ConsoleStorage : public Storage
		{
			public:

				virtual bool Initialise() { return true; }

				virtual void Store(Aggregation &aggregation)
				{
					std::cout << ent::entity::encode<ent::prettyjson>(aggregation) << std::endl;
				}
		};


		/// Aggregated results will be stored to Redis. The results are serialised to JSON and
		/// then stored at the key "profile:ID" where ID is the with which the Aggregator singleton
		/// was initialised.
		class RedisStorage : public Storage
		{
			public:

				RedisStorage(bool socket = false, std::string connection = "127.0.0.1", int port = 6379)
					: port(port), socket(socket), connection(connection) {}


				virtual bool Initialise()
				{
					return this->cache.Initialise(socket, connection, port);
				}


				virtual void Store(Aggregation &aggregation)
				{
					this->cache.Set(
						"profile:" + aggregation.id,
						ent::entity::encode<ent::json>(aggregation)
					);
				}


			private:

				redis::Redis cache;

				int port;
				bool socket;
				std::string connection;
		};


		/// Aggregated results are serialsed to JSON and stored in files based on the
		/// ID specified when initialising the Aggregator singleton. If the directory
		/// declared does not exist then initialisation will fail and results will
		/// not be generated.
		class FileStorage : public Storage
		{
			public:

				FileStorage(Path directory) : directory(directory) {}

				virtual bool Initialise()
				{
					return directory.exists();
				}


				virtual void Store(Aggregation &aggregation)
				{
					String::save(
						directory / String::format("profile_%s.json", aggregation.id),
						ent::entity::encode<ent::json>(aggregation)
					);
				}

			private:

				Path directory;
		};
	}
}
