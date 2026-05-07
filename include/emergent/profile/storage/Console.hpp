#pragma once

#include <emergent/profile/Storage.hpp>
#include <entity/json.hpp>
#include <iostream>


namespace emergent
{
	namespace profile
	{
	/// Simple storage which writes a pretty-printed JSON version
		/// of the aggregated results to the console.
		class ConsoleStorage : public Storage
		{
			public:

				bool Initialise() override { return true; }

				void Store(Aggregation &aggregation) override
				{
					std::cout << ent::encode<ent::prettyjson>(aggregation) << std::endl;
				}
		};
	}
}
