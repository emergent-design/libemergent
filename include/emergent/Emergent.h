#pragma once

#include <string>
#include <vector>
#include <functional>

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

	/// Generate a hyphenated string from a camel-cased one
	const std::string hyphenate(std::string text);

	/// Split a string into a list of strings wherever the specified delimiters are found
	std::vector<std::string> explode(const std::string &text, const std::string &delimiters);

	typedef std::function<bool(int)> event;
}

namespace emg = emergent;
