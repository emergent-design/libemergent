#pragma once

#include <map>
#include <set>
#include <sstream>
#include <functional>
#include <emergent/Path.hpp>
#include <emergent/String.hpp>

#ifdef __linux
	#include <sys/ioctl.h>
#endif


namespace emergent
{
	// A command-line argument parser.
	class Clap
	{
		public:

			// Helper for defining an option in a fluent manner
			struct Option
			{
				// The long name of this option.
				// e.g. "help" which will expect "--help" on the command-line
				auto &Name(std::string name)
				{
					this->name = name;
					return *this;
				}

				// A description for this option that will appear in the
				// usage text.
				auto &Describe(std::string description)
				{
					this->description = description;
					return *this;
				}

				// Bind this option to a variable. This may be a numeric
				// or a string. Additionally it can be a std::vector of
				// numeric or string.
				template<typename T> auto &Bind(T &item)
				{
					this->set = [&](auto v) { Convert(v, item); };
					return *this;
				}

				// Bind this option to a function that will be invoked
				// with the corresponding value if the option is supplied.
				auto &Bind(std::function<void(std::string)> set)
				{
					this->set = set;
					return *this;
				}

				// Bind this option to a flag. All options of this type
				// do not expect a corresponding value on the command-line.
				auto &Bind(bool &item)
				{
					this->flag	= true;
					this->set	= [&](auto) { item = true; };
					item		= false;
					return *this;
				}


				bool flag = false;
				std::string name;
				std::string description;
				std::function<void(std::string)> set = nullptr;
			};


			// Return an option to be configured for a specific short name.
			// An exception will be thrown if an option with the same short
			// name has already been specified.
			Option &operator [](char id)
			{
				if (this->options.count(id))
				{
					throw std::runtime_error(String::format("Duplicate option type: %c", id));
				}

				return this->options[id];
			}


			// Return a positional argument to be configured. An exception will
			// be thrown if the position has already been specified.
			Option &operator [](int position)
			{
				if (this->positions.count(position))
				{
					throw std::runtime_error(String::format("Duplicate positional argument: %d", position));
				}

				return this->positions[position];
			}


			// Parse the command-line. Any unassigned positional arguments
			// will be returned in a vector.
			auto Parse(int argc, char *argv[])
			{
				// Check that each long option name only occurs once
				std::set<std::string> names;
				for (auto &o : options)
				{
					if (!o.second.name.empty() && !names.insert(o.second.name).second)
					{
						throw std::runtime_error("Duplicate long option name: " + o.second.name);
					}
				}

				std::vector<std::string> unused;
				auto items	= Split(argc, argv);
				int count	= items.size();
				int pos		= 1;

				for (int i=0; i<count; i++)
				{
					if (items[i].second)
					{
						this->ParseOption(i, items[i].first, i < count - 1 ? &items[i+1] : nullptr);
					}
					else
					{
						this->ParsePosition(pos++, items[i].first, unused);
					}
				}

				return unused;
			}


			// Return the usage description as a string.
			std::string Usage(std::string processName, int consoleWidth = 0)
			{
				std::ostringstream description;
				this->Usage(description, processName, consoleWidth);
				return description.str();
			}


			// Generate the usage description.
			void Usage(std::ostream &dst, std::string processName, int consoleWidth = 0)
			{
				int widest	= 0;
				int width	= consoleWidth ? consoleWidth : this->ConsoleWidth();
				std::vector<std::pair<std::string, std::string>> entries;

				dst << "usage: " << Path(processName).filename() << " [options]";

				// List and describe the positional arguments
				for (auto &p : positions)
				{
					auto name = p.second.name.empty()
						? String::format("  <arg%d>", p.first)
						: String::format("  <%s>", p.second.name);

					dst << name.substr(1);

					if (!p.second.description.empty())
					{
						entries.emplace_back(name, p.second.description);
						widest = std::max(widest, (int)name.size() + 2);
					}
				}

				dst << std::endl << std::endl;

				if (entries.size()) entries.emplace_back("", " ");

				// Generate the option signatures
				for (auto &o : this->options)
				{
					auto name	= o.second.name;
					auto entry	= String::format("  -%c", o.first);

					if (!name.empty())	entry += String::format(", --%s", name);
					if (!o.second.flag)	entry += name.empty() ? " <value>" : "=<value>";

					entries.emplace_back(entry, o.second.description);
					widest = std::max(widest, (int)entry.size() + 2);
				}

				// Describe the options
				for (auto &e : entries)
				{
					dst << e.first << std::string(widest - e.first.size(), ' ');

					for (auto &l : Format(e.second, widest, width - widest))
					{
						dst << l << std::endl;
					}
				}
			}


		private:

			// Determine the console width on linux systems.
			int ConsoleWidth()
			{
				#ifdef __linux
					#ifdef TIOCGSIZE
						struct ttysize ts;
						ioctl(STDIN_FILENO, TIOCGSIZE, &ts);
						return ts.ts_cols;
					#elif defined(TIOCGWINSZ)
						struct winsize ts;
						ioctl(STDIN_FILENO, TIOCGWINSZ, &ts);
						return ts.ws_col;
					#endif
				#endif

				return 80;
			}


			// Split the command-line arguments up into a list of option names and
			// values. The bool indicates whether it is an option or a value.
			static  const std::vector<std::pair<std::string, bool>> Split(int argc, char *argv[])
			{
				std::vector<std::pair<std::string, bool>> items;

				for (int i=1; i<argc; i++)
				{
					std::string item = argv[i];

					if (item.size() > 1 && item[0] == '-')
					{
						if (item[1] == '-')
						{
							auto v = item.find_first_of('=');

							if (v != std::string::npos)
							{
								items.emplace_back(item.substr(2, v - 2), true);
								items.emplace_back(item.substr(v + 1), false);
							}
							else items.emplace_back(item.substr(2), true);
						}
						else
						{
							for (int j=1; j<item.size(); j++)
							{
								if (std::isalpha(item[j]))
								{
									items.emplace_back(std::string(1, item[j]), true);
								}
								else throw std::runtime_error("Invalid option: " + std::string(1, item[j]));
							}
						}
					}
					else items.emplace_back(item, false);
				}

				return items;
			}


			// Parse a single option, "next" contains the next item on the command-line (if
			// one exists).
			void ParseOption(int &index, std::string name, std::pair<std::string, bool> *next)
			{
				Option *option = nullptr;

				if (name.size() == 1)
				{
					// Single character name, so look up directly
					if (this->options.count(name[0])) option = &this->options[name[0]];
				}
				else
				{
					// Long name, so search the available options for a match
					for (auto &o : this->options)
					{
						if (name == o.second.name)
						{
							option = &o.second;
							break;
						}
					}
				}

				if (!option)		throw std::runtime_error("Unknown option: " + name);
				if (!option->set)	throw std::runtime_error("No variable bound to option: " + name);
				if (!option->flag)
				{
					// If this is not a flag then there must be a value to assign it.
					if (next && !next->second)
					{
						try
						{
							option->set(next->first);
							index++;
						}
						catch (...)
						{
							throw std::runtime_error("Invalid value for option: " + name);
						}
					}
					else throw std::runtime_error("Expected value for option: " + name);
				}
				else option->set("");
			}


			// Parse a positional argument. Any arguments in positions that have not been defined
			// will be added to the unused vector.
			void ParsePosition(int pos, std::string value, std::vector<std::string> &unused)
			{
				if (this->positions.count(pos))
				{
					if (this->positions[pos].set)
					{
						this->positions[pos].set(value);
					}
					else
					{
						throw std::runtime_error(String::format("No variable bound to position: %d", pos));
					}
				}
				else unused.push_back(value);
			}


			// Format the description of a single option to fit the console width. The value
			// "padding" is the amount of indentation required for multi-line descriptions.
			static const std::vector<std::string> Format(std::string description, int padding, int width)
			{
				if (description.empty()) return {""};

				std::vector<std::string> lines;
				auto np		= std::string::npos;
				auto pad	= std::string(padding, ' ');
				int size	= description.size();
				int pos		= 0;

				while (pos < size)
				{
					auto last = size - pos < width ? np : description.find_last_of(" .,([/-", pos + width - 1);
					auto next = description.substr(pos, last == np || last < pos ? width : last + 1);

					lines.push_back((pos ? pad : "") + next);
					pos += next.size();
				}

				return lines;
			}


			// If the bound variable is a vector<> then attempt to convert the value
			// to the element type of the container and then add it.
			template <typename T> static void Convert(std::string value, std::vector<T> &target)
			{
				T converted;
				Convert(value, converted);
				target.push_back(converted);
			}


			// A string does not require conversion.
			static void Convert(std::string value, std::string &target)
			{
				target = value;
			}


			// Convert value to the appropriate numeric type.
			template <typename T> static void Convert(std::string value, T &target)
			{
				if (std::is_floating_point<T>::value)		target = std::stod(value);
				if (std::is_integral<T>::value)				target = std::stol(value);
			}

			std::map<char, Option> options;
			std::map<int, Option> positions;
	};
}

