#pragma once

#include <emergent/Maths.h>
#include <ostream>


namespace emergent
{
	/// Utility structure for storing value bounds
	template <class T> struct bounds
	{
		T min	= 0;
		T max	= 0;
		T range	= 0;

		/// Default constructor
		bounds() {}

		/// Constructor that initialises the values
		bounds(T min, T max) { this->set(min, max); }

		/// Updates the values
		void set(T min, T max) { this->min = min; this->max = max; this->range = max - min; }

		/// Determine if the given value lies within the bounds
		inline bool contains(T value) { return value >= this->min && value <= this->max; }

		/// Clamp the given value within the range defined by these bounds
		inline T clamp(T value) { return value < this->min ? this->min : value > this->max ? this->max : value; }

		/// Normalise a given value using the bounds
		inline double normalise(T value) { return (this->range > 0) ? (double)(value - this->min) / (double)this->range : 0.0; }

		/// Produces a random value within the given bounds
		inline T random() { return this->min + (T)((double)this->range * nrand()); }
	};
}



template <class T> std::ostream &operator << (std::ostream &output, const emergent::bounds<T> &b)
{
	return output << "min=" << b.min << ", max=" << b.max;
}
