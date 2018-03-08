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
}

/// Shortened form of the namespace
namespace emg = emergent;
