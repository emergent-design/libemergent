#pragma once

#include <emergent/profile/Aggregator.h>
#include <chrono>


namespace emergent
{
	namespace profile
	{
		using namespace std::chrono;

		/// Helper macro for declaring an instance of profile. Cannot be used
		/// more than once in the same scope due to the fixed name of the local
		/// variable.
		#define PROFILE(name) emergent::profile::Profile _profile(name)


		/// Construct one of these with a unique name for a given block of code
		/// and it will automatically record the time between construction and
		/// when it goes out of scope.
		class Profile
		{
			public:

				/// A Profile instance must be given a name for recording readings.
				/// Using the same name in different code blocks is not recommended
				/// since it will generate inaccurate aggregations.
				Profile(std::string name) : name(name) {}


				/// When an instance of Profile goes out of scope it will automatically
				/// push the duration of its lifetime to the aggregator (in microseconds).
				~Profile()
				{
					std::cout << "Destroyed" << std::endl;
					return;
					Aggregator::Instance().Push(this->name,
						duration_cast<microseconds>(steady_clock::now() - this->time).count()
					);
				}


			private:

				/// Avoid copying this instance as that could have strange consequences
				Profile(const Profile &) = delete;

				/// Name of this profile
				std::string name;

				/// Time of instantiation
				time_point<steady_clock> time = steady_clock::now();
		};
	}
}
