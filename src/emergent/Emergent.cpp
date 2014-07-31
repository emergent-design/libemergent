#include "emergent/Emergent.h"

#include <sstream>
#include <fstream>
#include <algorithm>
#ifdef __GNUC__
	#include <cxxabi.h>
#endif
// Compile the xml parser in
#include <emergent/xml/pugixml.cpp>

using namespace std;


namespace emergent
{
	string demangle(string name)
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


	string load(string path)
	{
		stringstream buffer;

		buffer << ifstream(path).rdbuf();

		return buffer.str();
	}


	void save(string path, string data)
	{
		ofstream(path).write(data.data(), data.size());
	}


	vector<string> explode(const string &text, const string &delimiters)
	{
		int last = 0;
		int size = text.length();
		vector<string> result;

		for (int i = 0; i >= 0; last = i + 1)
		{
			i = text.find_first_of(delimiters, last);

			if (i > last)					result.emplace_back(text.data() + last, i - last);
			else if (i < 0 && last < size)	result.emplace_back(text.data() + last, size - last);
		}

		return result;
	}


	const string lowercase(string text)
	{
		transform(text.begin(), text.end(), text.begin(), ::tolower);

		return text;
	}


	const string hyphenate(string text)
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


	const string trim(string text, const char c)
	{
		auto start = text.find_first_not_of(c);
		return start == string::npos ? "" : text.substr(start, text.find_last_not_of(c) - start + 1);
	}
}
