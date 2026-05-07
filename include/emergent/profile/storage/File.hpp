#pragma once

#include <emergent/profile/Storage.hpp>
#include <emergent/String.hpp>
#include <entity/json.hpp>
#include <filesystem>


namespace emergent
{
	namespace profile
	{
		/// Aggregated results are serialsed to JSON and stored in files based on the
		/// ID specified when initialising the Aggregator singleton. If the directory
		/// declared does not exist then initialisation will fail and results will
		/// not be generated.
		class FileStorage : public Storage
		{
			public:

				FileStorage(const std::filesystem::path &directory) : directory(directory) {}

				bool Initialise() override
				{
					return std::filesystem::exists(this->directory)
						&& std::filesystem::is_directory(this->directory);
				}


				void Store(Aggregation &aggregation) override
				{
					String::save(
						directory / String::format("profile_%s.json", aggregation.id),
						ent::encode<ent::json>(aggregation)
					);
				}

			private:

				std::filesystem::path directory;
		};
	}
}
