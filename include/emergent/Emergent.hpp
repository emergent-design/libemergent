#pragma once

#include <string>
#include <vector>
#include <functional>
#include <emergent/String.hpp>


/// When dealing with binary data and images the unsigned char type is
/// heavily used and byte is much easier to type. It really should be
/// a default typedef in C++.
typedef unsigned char byte;


namespace emergent
{
	/// Simple event definition, passes an integer status and expects a bool in return
	typedef std::function<bool(int)> event;
}

/// Shortened form of the namespace
namespace emg = emergent;
