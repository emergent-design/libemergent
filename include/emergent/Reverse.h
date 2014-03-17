#pragma once


namespace emergent
{
	/// Helper class that allows you to easily use a
	/// range-based for loop where you need loop through
	/// the items in reverse. For example:
	///
	/// for (auto &i : reverse(items)) ...
	template<class T> class reverse_wrapper
	{
			T& container;
			
		public:
			reverse_wrapper(T& cont) : container(cont) {}
			decltype(container.rbegin()) begin()	{ return container.rbegin();	}
			decltype(container.rend()) end()		{ return container.rend();		}
	};


	template<class T> reverse_wrapper<T> reverse(T& cont)
	{
		return reverse_wrapper<T>(cont);
	}
}