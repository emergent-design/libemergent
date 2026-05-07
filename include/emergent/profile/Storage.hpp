#pragma once

#include <emergent/profile/Aggregation.hpp>


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
	}
}
