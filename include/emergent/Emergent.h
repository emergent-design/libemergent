#pragma once

#include <string>
#include <vector>
#include <functional>


/// When dealing with binary data and images the unsigned char type is
/// heavily used and byte is much easier to type. It really should be
/// a default typedef in C++.
typedef unsigned char byte;


namespace emergent
{
	/// Demangle a function name, useful for logging purposes.
	std::string demangle(std::string name);

	/// Load a whole file into a string
	std::string load(std::string path);

	/// Save a string to a file
	void save(std::string path, std::string data);

	/// Change a string to lowercase (transforms the string that is passed in).
	const std::string lowercase(std::string text);

	/// Generate a lowercase hyphenated string from a camel-cased one
	const std::string hyphenate(std::string text);

	/// Trim a string (both ends) of the given character
	const std::string trim(std::string text, const char c);

	/// Split a string into a list of strings wherever the specified delimiters are found
	std::vector<std::string> explode(const std::string &text, const std::string &delimiters);

	/// Simple event definition, passes an integer status and expects a bool in return
	typedef std::function<bool(int)> event;
}

/// Shortened form of the namespace
namespace emg = emergent;
