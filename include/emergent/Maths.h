#pragma once

#include <emergent/Emergent.h>
#include <cmath>
#include <limits>


namespace emergent
{
	class Maths
	{

		public:

			/// Helper function used to clamp a value within the numeric range of the required type
			template <class T> static inline T clamp(long value)
			{
				return (value > std::numeric_limits<T>::max()) ? std::numeric_limits<T>::max() : (value < std::numeric_limits<T>::min()) ? std::numeric_limits<T>::min() : value;
			}

			template <class T> static inline T clamp(unsigned long value)
			{
				return (value > std::numeric_limits<T>::max()) ? std::numeric_limits<T>::max() : (value < std::numeric_limits<T>::min()) ? std::numeric_limits<T>::min() : value;
			}

			template <class T> static inline T clamp(int value)
			{
				return (value > std::numeric_limits<T>::max()) ? std::numeric_limits<T>::max() : (value < std::numeric_limits<T>::min()) ? std::numeric_limits<T>::min() : value;
			}

			template <class T> static inline T clamp(double value)
			{
				if (std::is_floating_point<T>::value)
				{
					return (value > std::numeric_limits<T>::max()) ? std::numeric_limits<T>::max() : (value < std::numeric_limits<T>::min()) ? std::numeric_limits<T>::min() : (T)value;
				}
				else
				{
					long v = lrint(value);

					return (v > std::numeric_limits<T>::max()) ? std::numeric_limits<T>::max() : (v < std::numeric_limits<T>::min()) ? std::numeric_limits<T>::min() : (T)v;
				}
			}


// template <class T, class = typename std::enable_if<std::is_arithmetic<T>::value>::type> tree(const T &value) :
	//			type(std::is_floating_point<T>::value ? Type::Floating : Type::Integer),
	//			leaf(new container<typename std::conditional<std::is_floating_point<T>::value, double, long>::type>(value)) {}

			// These are not actually faster!!
			// /// Specialisation of the clamp function for byte values (this implementation is faster than
			// /// the general one which is important because this version will get used a lot)
			// template <> inline byte clamp(long value) 			{ return (value > 255) ? 255 : (value < 0) ? 0 : value; }
			// template <> inline byte clamp(unsigned long value)	{ return (value > 255) ? 255 : value; }
			// template <> inline byte clamp(int value)			{ return (value > 255) ? 255 : (value < 0) ? 0 : value; }
			//template <> inline byte clamp(double value) 		{ long v = lrint(value); return (v > 255) ? 255 : (v < 0) ? 0 : v; }

			// /// Specialisation of the clamp function for char values
			// template <> inline char clamp(long value) 			{ return (value > 127) ? 127 : (value < -127) ? -127 : value; }
			// template <> inline char clamp(unsigned long value)	{ return (value > 127) ? 127 : value; }
			// template <> inline char clamp(int value) 			{ return (value > 127) ? 127 : (value < -127) ? -127 : value; }
			// template <> inline char clamp(double value) 		{ long v = lrint(value); return (v > 127) ? 127 : (v < -127) ? -127 : v; }

			/// Lookup table for cosine values where the index is the angle in degrees (0-360)
			static const double COS[361];

			/// Lookup table for sine values where the index is the angle in degrees (0-360)
			static const double SIN[361];

			/// Normalised random number
			static inline double nrand()
			{
				return (double)rand() / (double)RAND_MAX;
			}
	};


	template <> inline long Maths::clamp(long value)					{ return value; }
	template <> inline unsigned long Maths::clamp(unsigned long value)	{ return value; }
	template <> inline int Maths::clamp(int value)						{ return value; }
	template <> inline double Maths::clamp(double value)				{ return value; }
}

