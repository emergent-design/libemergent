#pragma once

#include <sstream>
#include <fstream>
#include <algorithm>

#ifdef __GNUC__
	#include <cxxabi.h>
#endif


namespace emergent
{
	using std::string;

	// String helper functions
	class String
	{
		public:

			/// Demangle a function name, useful for logging purposes.
			static string demangle(const string name)
			{
				#ifdef __GNUC__
					char *demangled	= abi::__cxa_demangle(name.c_str(), 0, 0, 0);
					string result	= demangled;
					free(demangled);
					return result;
				#else
					return name;
				#endif
			}


			/// Load a whole file into a string
			static string load(string path)
			{
				std::string result;
				std::ifstream in(path, std::ios::in | std::ios::binary);

				if (in)
				{
					result.resize(in.seekg(0, std::ios::end).tellg());
					in.seekg(0, std::ios::beg).read(&result[0], result.size());
					in.close();
				}

				return result;
			}


			/// Save a string to a file
			static void save(string path, string data)
			{
				std::ofstream(path).write(data.data(), data.size());
			}


			/// Split a string into a list of strings wherever the specified delimiters are found
			static std::vector<string> explode(const string &text, const string &delimiters)
			{
				int last = 0;
				int size = text.length();
				std::vector<string> result;

				for (int i = 0; i >= 0; last = i + 1)
				{
					i = text.find_first_of(delimiters, last);

					if (i > last)					result.emplace_back(text.data() + last, i - last);
					else if (i < 0 && last < size)	result.emplace_back(text.data() + last, size - last);
				}

				return result;
			}


			/// Change a string to lowercase (transforms the string that is passed in).
			static const string lowercase(string text)
			{
				transform(text.begin(), text.end(), text.begin(), ::tolower);

				return text;
			}


			/// Generate a lowercase hyphenated string from a camel-cased one
			static const string hyphenate(const string text)
			{
				string result;
				result.reserve(text.length());

				for (int i=0; i<text.size(); i++)
				{
					if (isupper(text[i]))
					{
						if (i) result.append("-");
						result.append(1, tolower(text[i]));
					}
					else result.append(1, text[i]);
				}

				return result;
			}


			/// Trim a string (both ends) of the given character
			static const string trim(const string text, const char c)
			{
				auto start = text.find_first_not_of(c);
				return start == string::npos ? "" : text.substr(start, text.find_last_not_of(c) - start + 1);
			}
	};
}
