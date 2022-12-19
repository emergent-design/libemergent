#pragma once
#include <iterator>


namespace emergent::image
{
	// An iterator for images - uses an arbitrary step size which can
	// be used by ImageBase to create an iterator for both rows or columns
	// It provides a pointer to each pixel in the row or column so that
	// all of the channels to be accessed at once.
	template <typename T> struct Iterator
	{
		T *data;
		const size_t count;
		const size_t step;

		template <typename U> struct iterator
		{
			using iterator_category = std::random_access_iterator_tag;
			using value_type		= U*;
			using difference_type	= std::ptrdiff_t;
			using pointer			= U**;
			using reference			= U*&;

			value_type data;
			const size_t step;

			explicit iterator(const value_type &data, const size_t &step) : data(data), step(step) {}

			value_type operator*() const							{ return data; }
			value_type operator[](const difference_type rhs) const	{ return data + rhs * step; }
			pointer operator->() const								{ return &data; }

			iterator &operator +=(const difference_type rhs) { data += rhs * step; return *this; }
			iterator &operator -=(const difference_type rhs) { data -= rhs * step; return *this; }

			iterator &operator++()		{ data += step; return *this; }
			iterator &operator--()		{ data -= step; return *this; }
			iterator operator++(int)	{ iterator tmp(*this); data += step; return tmp; }
			iterator operator--(int)	{ iterator tmp(*this); data -= step; return tmp; }

			difference_type operator-(const iterator &rhs) const { return (data - rhs.data) / step; }
			iterator operator+(const difference_type &rhs) const { return iterator(data + rhs * step, step); }
			iterator operator-(const difference_type &rhs) const { return iterator(data - rhs * step, step); }

			bool operator==(const iterator &rhs) const	{ return data == rhs.data; }
			bool operator!=(const iterator &rhs) const	{ return data != rhs.data; }
			bool operator>(const iterator &rhs) const	{ return data > rhs.data; }
			bool operator<(const iterator &rhs) const	{ return data < rhs.data; }
			bool operator>=(const iterator &rhs) const	{ return data >= rhs.data; }
			bool operator<=(const iterator &rhs) const	{ return data <= rhs.data; }
		};

		Iterator()													: data(nullptr), count(0), step(1)		{}
		Iterator(T *data, const size_t &count, const size_t &step)	: data(data), count(count), step(step)	{}

		iterator<T> begin() { return iterator<T>(data, step); }
		iterator<T> end()	{ return iterator<T>(data + count * step, step); }
		iterator<const T> begin() const { return iterator<const T>(data, step); }
		iterator<const T> end()	const	{ return iterator<const T>(data + count * step, step); }
	};

	// Region iterator?
}
