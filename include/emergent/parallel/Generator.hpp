#pragma once
#include <iterator>


namespace emergent
{
	// Lazy generation of an integral number sequence. Useful for the C++17 parallel execution functions
	// when dealing with images since it avoids creating lookup vectors.
	template <typename T> struct Generator
	{
		static_assert(std::is_integral_v<T>, "Generator must use an integral type");

		const T first;
		const T count;
		const T step;

		struct iterator
		{
			using iterator_category	= std::random_access_iterator_tag;
			using value_type		= T;
			using difference_type	= std::ptrdiff_t;
			using size_type			= size_t;
			using pointer			= T*;
			using reference			= T&;

			T value;
			T step;


			explicit iterator(const T &value, const T &step) : value(value), step(step) {}

			const T &operator*() const							{ return value; }
			T operator[](const difference_type rhs) const	{ return value + rhs * step; }
			T *operator->() const							{ return &value; }

			iterator &operator+=(const difference_type rhs) { value += rhs * step; return *this; }
			iterator &operator-=(const difference_type rhs) { value -= rhs * step; return *this; }

			iterator &operator++()		{ value += step; return *this; }
			iterator &operator--()		{ value -= step; return *this; }
			iterator operator++(int)	{ iterator tmp(*this); value += step; return tmp; }
			iterator operator--(int)	{ iterator tmp(*this); value -= step; return tmp; }

			difference_type operator-(const iterator &rhs) const { return (value - rhs.value) / step; }
			iterator operator+(const difference_type rhs) const { return iterator(value + rhs * step, step); }
			iterator operator-(const difference_type rhs) const { return iterator(value - rhs * step, step); }


			bool operator==(const iterator &rhs) const	{ return value == rhs.value; }
			bool operator!=(const iterator &rhs) const	{ return value != rhs.value; }
			bool operator>(const iterator &rhs) const	{ return value > rhs.value; }
			bool operator<(const iterator &rhs) const	{ return value < rhs.value; }
			bool operator>=(const iterator &rhs) const	{ return value >= rhs.value; }
			bool operator<=(const iterator &rhs) const	{ return value <= rhs.value; }

		};

		Generator() 													: first(0), count(0), step(1) {}
		Generator(const T &first, const T &count, const T &step = 1)	: first(first), count(count), step(step) {}

		iterator begin() const	{ return iterator(first, step); }
		iterator end() const	{ return iterator(first + count * step, step); }
	};

}
