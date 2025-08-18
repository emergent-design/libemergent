#pragma once

#include <cstdint>
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
	// actually guarantee a contiguous buffer but in c++20 we can use the contiguous_iterator concept
	#if __cplusplus > 201703L
		template <typename T> inline constexpr bool is_contiguous = std::contiguous_iterator<typename T::iterator>;
	#else
		template <typename T> inline constexpr bool is_contiguous = std::is_base_of<std::random_access_iterator_tag,
			typename std::iterator_traits<decltype(std::begin(std::declval<T const>()))>::iterator_category
		>::value;
	#endif
}

/// Shortened form of the namespace
namespace emg = emergent;
