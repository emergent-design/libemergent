#pragma once

#include <string>
#include <vector>
#include <functional>
#include <emergent/String.hpp>


namespace emergent
{
	/// For dealing with binary data and images
	typedef uint8_t byte;

	/// Simple event definition, passes an integer status and expects a bool in return
	typedef std::function<bool(int)> event;

	// Check that a container is a contiguous buffer. Be aware that random_aceess_iterator_tag doesn't
	// actually guarantee a contiguous buffer but in c++20 we can use contiguous_iterator_tag
	template <typename T> inline constexpr bool is_contiguous = std::is_base_of<
		#if __cplusplus > 201703L
			std::contiguous_iterator_tag,
		#else
			std::random_access_iterator_tag,
		#endif
		typename std::iterator_traits<
			decltype(std::begin(std::declval<T const>()))
		>::iterator_category
	>::value;
}

/// Shortened form of the namespace
namespace emg = emergent;
