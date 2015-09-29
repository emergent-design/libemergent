#pragma once

#include <vector>


namespace emergent
{
	// A simple wrapper for a std::map<> that allows you to easily iterate
	// over the keys in that map. For example:
	//   for (auto &k : KeyIterator<decltype(mymap)>(mymap)) ...
	template <typename T> class KeyIterator
	{
		public:

			struct iterator : std::iterator<std::input_iterator_tag, typename T::key_type>
			{
				iterator() {}
				iterator(typename T::const_iterator i)	: current(i) {}
				iterator(const iterator &i)				: current(i.current) {}
				iterator operator++(int)				{ return iterator(this->current++);	}
				iterator &operator++()					{ this->current++; return *this; }
				bool operator!=(const iterator &i) 		{ return i.current != this->current; }
				bool operator==(const iterator &i)		{ return i.current == this->current; }
				const typename T::key_type &operator*()	{ return this->current->first; }

				private:
					typename T::const_iterator current;
			};

			iterator begin() const	{ return { src.begin() }; }
			iterator end() const	{ return { src.end() }; }

			KeyIterator(T &src) : src(src) {}

		private:

			T &src;
	};

	// A helper function for the wrapper to clean up the syntax:
	//   for (auto &k : key_iterator(mymap)) ...
	template <typename T> KeyIterator<T> key_iterator(T &src)
	{
		return KeyIterator<T>(src);
	}
}
