#pragma once

#include <emergent/Emergent.hpp>
#include <random>



namespace emergent
{
	struct uuid
	{
		uint64_t a;
		uint64_t b;

		uuid()
		{
			static thread_local bool seeded = false;
			static thread_local std::mt19937_64 engine;
			static thread_local std::uniform_int_distribution<uint64_t> dist;

			if (!seeded)
			{
				// Seed the engine once per thread since excessive use
				// of random_device will result in a severe performance
				// drop (entropy pool exhaustion).
				auto seed = std::random_device()();

				#ifdef __MINGW32__
					// Since mingw does not implement std::random_device properly, retrieve
					// a seed using the system time.
					seed = time(0);
				#endif

				engine.seed(seed);

				seeded = true;
			}

			this->a	= (dist(engine) & 0xffffffffffff0fffULL) | 0x0000000000004000ULL;
			this->b	= (dist(engine) & 0x3fffffffffffffffULL) | 0x8000000000000000ULL;
		}


		std::string to_string()
		{
			static const char *NIBBLE = "0123456789abcdef";

			int i;
			char result[]	= "------------------------------------";
			auto a			= this->a;
			auto b			= this->b;
			auto *p			= result + 35;

			for (i=0;		i<12;	i++, b = b >> 4)	*p-- = NIBBLE[b & 0x0f];
			for (i=0, p--;	i<4;	i++, b = b >> 4)	*p-- = NIBBLE[b & 0x0f];
			for (i=0, p--;	i<4;	i++, a = a >> 4)	*p-- = NIBBLE[a & 0x0f];
			for (i=0, p--;	i<4;	i++, a = a >> 4)	*p-- = NIBBLE[a & 0x0f];
			for (i=0, p--;	i<8;	i++, a = a >> 4)	*p-- = NIBBLE[a & 0x0f];

			return result;
		}


		std::vector<byte> to_binary()
		{
			int i;
			std::vector<byte> result(16);

			auto a	= this->a;
			auto b	= this->b;
			auto *p	= result.data() + 15;

			for (i=0; i<8; i++, b = b >> 8) *p-- = b & 0xff;
			for (i=0; i<8; i++, a = a >> 8) *p-- = a & 0xff;

			return result;
		}
	};
}
