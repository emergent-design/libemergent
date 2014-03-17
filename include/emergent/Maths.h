#pragma once

#include <emergent/Emergent.h>
#include <cmath>
#include <limits>


namespace emergent
{
	/// Helper function used to clamp a value within the numeric range of the required type
	template <class T> inline T clamp(long value)
	{
		return (value > std::numeric_limits<T>::max()) ? std::numeric_limits<T>::max() : (value < std::numeric_limits<T>::min()) ? std::numeric_limits<T>::min() : value;
	}

	template <class T> inline T clamp(unsigned long value)
	{
		return (value > std::numeric_limits<T>::max()) ? std::numeric_limits<T>::max() : (value < std::numeric_limits<T>::min()) ? std::numeric_limits<T>::min() : value;
	}

	template <class T> inline T clamp(int value)
	{
		return (value > std::numeric_limits<T>::max()) ? std::numeric_limits<T>::max() : (value < std::numeric_limits<T>::min()) ? std::numeric_limits<T>::min() : value;
	}

	template <class T> inline T clamp(double value)
	{
		return (value > std::numeric_limits<T>::max()) ? std::numeric_limits<T>::max() : (value < std::numeric_limits<T>::min()) ? std::numeric_limits<T>::min() : (T)value;
	}


	template <> inline long clamp(long value)					{ return value; }
	template <> inline unsigned long clamp(unsigned long value)	{ return value; }
	template <> inline int clamp(int value)						{ return value; }
	template <> inline double clamp(double value)				{ return value; }

	/// Specialisation of the clamp function for byte values (this implementation is faster than
	/// the general one which is important because this version will get used a lot)
	template <> inline byte clamp(long value) 			{ return (value > 255) ? 255 : (value < 0) ? 0 : value; }
	template <> inline byte clamp(unsigned long value)	{ return (value > 255) ? 255 : value; }
	template <> inline byte clamp(int value)			{ return (value > 255) ? 255 : (value < 0) ? 0 : value; }
	template <> inline byte clamp(double value) 		{ long v = lrint(value); return (v > 255) ? 255 : (v < 0) ? 0 : v; }

	/// Specialisation of the clamp function for char values
	template <> inline char clamp(long value) 			{ return (value > 127) ? 127 : (value < -127) ? -127 : value; }
	template <> inline char clamp(unsigned long value)	{ return (value > 127) ? 127 : value; }
	template <> inline char clamp(int value) 			{ return (value > 127) ? 127 : (value < -127) ? -127 : value; }
	template <> inline char clamp(double value) 		{ long v = lrint(value); return (v > 127) ? 127 : (v < -127) ? -127 : v; }

	extern const double COS[];	///< Lookup table for cosine values where the index is the angle in degrees (0-360)
	extern const double SIN[];	///< Lookup table for sine values where the index is the angle in degrees (0-360)

	/// Normalised random number
	double nrand();
}