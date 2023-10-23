#pragma once

#include <sstream>
#include <fstream>
#include <algorithm>
#include <functional>
#include <memory>
#include <numeric>
#include <vector>

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
			static string load(const string path)
			{
				std::string result;
				std::ifstream in(path, std::ios::in | std::ios::binary);

				if (in)
				{
					result.resize(in.seekg(0, std::ios::end).tellg());
					in.seekg(0, std::ios::beg).read(&result[0], result.size());
				}

				return result;
			}


			/// Save a string to a file
			static void save(const string path, const string &data)
			{
				std::ofstream(path).write(data.data(), data.size());
			}


			static std::string implode(const std::vector<std::string> &items, const std::string &delimiter)
			{
				return items.empty() ? "" : std::accumulate(
					std::next(items.begin()),
					items.end(),
					items.front(),
				[&](const auto &a, const auto &b) { return a + delimiter + b; }
				);
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

			#ifdef __cpp_lib_string_view
				#include <string_view>

				/// Split a string into a list of strings wherever the specified delimiters are found
				static std::vector<std::string_view> explode(std::string_view text, std::string_view delimiters)
				{
					int last = 0;
					int size = text.length();
					std::vector<std::string_view> result;

					for (int i=0; i>=0; last = i + 1)
					{
						i = text.find_first_of(delimiters, last);

						if (i > last)					result.emplace_back(text.data() + last, i - last);
						else if (i < 0 && last < size)	result.emplace_back(text.data() + last, size - last);
					}

					return result;
				}

				/// Interpolate tokens delimited by {} within the string text and replace using the substitution function - this
				/// allows lookups or modifications to be performed. For example:
				///		auto text = interpolate("The value is {value}\n", [](auto s) { return "hello"; });
				/// which will result in text containing "The value is hello\n"
				static const std::string interpolate(std::string_view text, std::function<const std::string(std::string_view)> substitute)
				{
					std::string result;
					result.reserve(text.length());

					size_t start	= text.find_first_of('{');
					size_t end		= 0;

					while (start != std::string_view::npos)
					{
						result += text.substr(end, start - end);
						end		= text.find_first_of('}', start);

						if (end == std::string_view::npos)
						{
							return {};	// Missing closing brace for token
						}

						result += substitute(
							text.substr(start + 1, end - start - 1)
						);

						start = text.find_first_of('{', ++end);
					}

					return end < text.length()
						? result.append(text.substr(end))
						: result;
				}
			#endif


			/// Change a string to lowercase (transforms the string that is passed in).
			static const string lowercase(string text)
			{
				transform(text.begin(), text.end(), text.begin(), ::tolower);

				return text;
			}


			/// Generate a lowercase hyphenated string from a camel-cased one
			static const string hyphenate(const string &text)
			{
				string result;
				result.reserve(text.length());

				for (size_t i=0; i<text.size(); i++)
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
			static const string trim(const string &text, const char c)
			{
				auto start = text.find_first_not_of(c);
				return start == string::npos ? "" : text.substr(start, text.find_last_not_of(c) - start + 1);
			}


			template <typename ...Args> static const string format(const char *format, Args ...args)
			{
				// Assume an initial buffer size of 1K
				const size_t reserve	= 1024;
				auto result				= std::string(reserve, 0);
				auto size				= snprintf(const_cast<char *>(result.data()), reserve, format, format_helper(args)...);

				// If the buffer was not big enough then resize and format again
				if (size >= reserve)
				{
					result.resize(size + 1);
					snprintf(const_cast<char *>(result.data()), size + 1, format, format_helper(args)...);
				}

				// Remove the null at the end
				result.resize(size);
				return result;
			}

		private:

			// For any standard type simply return the value for formatting, in the case of a string
			// retrieve the c_str instead since snprintf does not support std::string.
			template <typename T> static inline const T format_helper(const T &parameter)	{ return parameter; }
			static inline const char *format_helper(const std::string &parameter)			{ return parameter.c_str(); }
	};
}
