#pragma once

#include <cstring>
#include <map>
#include <set>
#include <sstream>
#include <functional>
#include <emergent/Console.hpp>
#include <emergent/String.hpp>
#include <emergent/FS.hpp>


namespace emergent
{
	// Tools to convert string parameters to other values.
	// It can also map a vector of strings to a tuple of required types which permits the following:
	//   const auto [dst, count] = parameters::get<fs::path, int>(parameters);
	namespace parameters
	{
		template <typename T> inline T transform(const std::string &s)
		{
			if (std::is_floating_point<T>::value)	return std::stod(s);
			if (std::is_integral<T>::value)			return std::stol(s, nullptr, 0);	// Allow for conversion of hex values
		}
		template <> inline bool transform(const std::string &s)		{ return s == "1" || s == "high" || s == "true"; }
		template <> inline fs::path transform(const std::string &s)	{ return s; }
		template <> inline string transform(const std::string &s)	{ return s; }


		template <typename... Types, std::size_t... Is> auto create_tuple(std::index_sequence<Is...>, const std::vector<std::string> &values)
		{
			return std::make_tuple(transform<Types>(values[Is])...);
		}

		template <typename... Types> std::tuple<Types...> get(const std::vector<string> &values)
		{
			if (values.size() != sizeof...(Types))
			{
				throw std::runtime_error("unexpected number of values");
			}

			return create_tuple<Types...>(std::index_sequence_for<Types...> {}, values);
		}


		template <typename T> T retrieve(const std::vector<std::string> &values, const size_t index, const T &def)
		{
			return index < values.size() ? transform<T>(values[index]) : def;
		}

		template <typename... Types, std::size_t... Is> auto create_tuple(std::index_sequence<Is...>, const std::vector<std::string> &values, Types... defaults)
		{
			return std::make_tuple(retrieve<Types>(values, Is, defaults)...);
		}

		template <typename... Types> std::tuple<Types...> get(const std::vector<string> &values, Types... defaults)
		{
			return create_tuple<Types...>(std::index_sequence_for<Types...> {}, values, defaults...);
		}
	}


	// Helpers for constructing a map of command-line operations. By deriving each of your ops from `operations::op<...Args>` they
	// can be described and invoked. `Return` is the return type for the run function and `...Args` are the arguments that will be
	// passed to the `run` function.
	//
	// Creating a map of operations is as simple as
	//   `cosnt auto ops = operations::create<First, Second, Third>();`
	// where each of the template arguments should be a type that derives from `operations::op<>`
	//
	// The operations can be described on the command-line by simply piping to cout
	//   `std::cout << op;`
	//
	// An operation is then run by name
	//   `ops.at("first")->run(arg0, arg1);`
	namespace operations
	{
		template <typename Return, typename... Args> struct op
		{
			using type = op<Return, Args...>;

			virtual ~op() {}
			virtual std::string_view name() const = 0;
			virtual std::string_view parameters() const = 0;
			virtual std::string_view description() const = 0;
			virtual Return run(Args...) = 0;
		};


		// Returns a map where each key is the `name()` of the op and the corresponding value is a shared_ptr to
		// the base `operations::op<>` class.
		// To determine the base class for all of the `Types` it first maps the types to a tuple, extracts the first type, and then
		// uses the `type` defined in the `op<>` base class.
		template <typename... Types> std::map<std::string, std::shared_ptr<typename std::tuple_element_t<0, std::tuple<Types...>>::type>> create()
		{
			auto make = [](auto a) { return std::make_pair(std::string { a->name() }, a); };

			return { make(std::make_shared<Types>())... };

			// return { [] {
			// 	auto instance = std::make_shared<Types>();
			// 	return std::make_pair(std::string { instance->name }, instance);
			// }()... };
		}


		// Opinionated formatting for printing out a map of operations. It will produce output that looks as follows
		//
		//   operations
		//
		//     first <parameters>
		//           description of the "first" operation
		//
		//    second <parameters>
		//           description of the "second" operation - this can
		//           support multi-line strings.
		//
		template <typename... Args> inline std::ostream &operator << (std::ostream &dst, const std::map<std::string, std::shared_ptr<op<Args...>>> &ops)
		{
			const size_t width	= Console::Width();
			size_t widest 		= 0;

			for (auto &[k, _] : ops)
			{
				widest = std::max(widest, k.size() + 2);
			}

			dst << "\noperations:\n\n";

			for (auto &[k, o] : ops)
			{
				dst	<< std::string(widest - k.size(), ' ')
					<< Console::Green << k << ' '
					<< Console::Yellow << Console::Format(o->parameters(), widest + 1, width) << '\n'
					<< Console::Reset << std::string(widest + 1, ' ')
					<< Console::Format(o->description(), widest + 1, width)
					<< '\n';
			}

			return dst;
		}
	}


	// A command-line argument parser.
	class Clap
	{
		public:

			// Helper for defining an option in a fluent manner
			struct Option
			{
				// The long name of this option.
				// e.g. "help" which will expect "--help" on the command-line
				auto &Name(const std::string &name)
				{
					this->name = name;
					return *this;
				}

				// A description for this option that will appear in the
				// usage text.
				auto &Describe(const std::string &description)
				{
					this->description = description;
					return *this;
				}


				// A formatted description for this option that will appear
				// in the usage text. Useful for including default values.
				template<typename ...Args> auto &Describe(const char *message, Args ...args)
				{
					this->description = String::format(message, args...);
					return *this;
				}


				// Bind this option to a variable. This may be a numeric
				// or a string. Additionally it can be a std::vector of
				// numeric or string.
				template<typename T> auto &Bind(T &item)
				{
					this->set = [&](auto v) { item = parameters::transform<T>(v); }; //Convert(v, item); };
					return *this;
				}


				// Bind this option to a vector - the set function can be
				// called multiple times and it will add each converted item
				// to the vector.
				template<typename T> auto &Bind(std::vector<T> &item)
				{
					this->set = [&](auto v) { item.push_back(parameters::transform<T>(v)); };
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
				std::function<void(const std::string &)> set = nullptr;
			};


			// Return an option to be configured for a specific short name.
			// An exception will be thrown if an option with the same short
			// name has already been specified.
			Option &operator [](char id)
			{
				if (id == '-')
				{
					this->longOptions.emplace_back();
					return this->longOptions.back();
				}

				if (std::isalnum(id))
				{
					if (this->options.count(id))
					{
						throw std::runtime_error(String::format("Duplicate option type: %c", id));
					}

					return this->options[id];
				}

				throw std::runtime_error(String::format("Invalid option type: %c", id));
			}


			// Return a positional argument to be configured. An exception will
			// be thrown if the position has already been specified. Positions
			// start at 1, but a position of 0 acts as a catch all for any positions
			// that are not defined. Therefore it is recommended that position 0 is
			// bound to a vector<>.
			Option &operator [](int position)
			{
				if (position < 0)
				{
					throw std::runtime_error(String::format("Invalid positional argument: %d", position));
				}
				if (this->positions.count(position))
				{
					throw std::runtime_error(String::format("Duplicate positional argument: %d", position));
				}

				return this->positions[position];
			}


			// Parse the command-line. If ignoreUnknowns is set then
			// it will not throw exceptions when dealing with unknown
			// options.
			void Parse(int argc, char *argv[], bool ignoreUnknowns = false)
			{
				this->SelfCheck();

				auto items	= Split(argc, argv);
				int count	= items.size();
				int pos		= 1;

				for (int i=0; i<count; i++)
				{
					if (items[i].second)
					{
						this->ParseOption(i, items[i].first, i < count - 1 ? &items[i+1] : nullptr, ignoreUnknowns);
					}
					else
					{
						this->ParsePosition(pos++, items[i].first);
					}
				}
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
				int width	= consoleWidth ? consoleWidth : Console::Width();

				std::vector<std::tuple<std::string_view, std::string, std::string_view>> entries;
				std::string extra;

				dst << "usage: " << fs::path(processName).filename().string() << Console::Cyan << " [options]" << Console::Yellow;

				// List and describe the positional arguments
				for (auto &[pos, opt] : positions)
				{
					auto name = opt.name.empty()
						? String::format("  <arg%d>", pos)
						: String::format("  <%s>", opt.name);

					if (pos == 0)	extra = name.substr(1) + "...";
					else			dst << name.substr(1);

					if (!opt.description.empty())
					{
						entries.emplace_back(Console::Yellow, name, opt.description);
						widest = std::max(widest, (int)name.size() + 2);
					}
				}

				dst << extra << Console::Reset << std::endl << std::endl;

				if (entries.size())
				{
					entries.emplace_back("", "", " ");
				}

				// Generate the option signatures
				for (auto &[c, opt] : this->options)
				{
					auto name	= opt.name;
					auto entry	= String::format("  -%c", c);

					if (!name.empty())	entry += String::format(", --%s", name);
					if (!opt.flag)		entry += name.empty() ? " <value>" : "=<value>";

					entries.emplace_back(Console::Cyan, entry, opt.description);
					widest = std::max(widest, (int)entry.size() + 2);
				}

				for (auto &opt : this->longOptions)
				{
					auto entry = String::format("      --%s%s", opt.name, opt.flag ? "" : "=<value>");
					entries.emplace_back(Console::Cyan, entry, opt.description);
					widest = std::max(widest, (int)entry.size() + 2);
				}

				// Describe the options
				for (auto &[colour, name, desc] : entries)
				{
					dst << colour << name << Console::Reset
						<< std::string(widest - name.size(), ' ')
						<< Console::Format(desc, widest, width)
					;
				}
			}


		private:

			// Split the command-line arguments up into a list of option names and
			// values. The bool indicates whether it is an option or a value.
			static const std::vector<std::pair<std::string, bool>> Split(int argc, char *argv[])
			{
				std::vector<std::pair<std::string, bool>> items;

				for (int i=1; i<argc; i++)
				{
					std::string item = argv[i];

					if (item.size() > 1 && item[0] == '-')
					{
						if (std::isdigit(item[1]))
						{
							// This is a negative number value not an option
							items.emplace_back(item, false);
						}
						else if (item[1] == '-')
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
							for (int j=1; j<(int)item.size(); j++)
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


			// Check that each long option name only occurs once
			void SelfCheck()
			{
				std::set<std::string> names;

				for (auto &o : this->options)
				{
					if (!o.second.name.empty() && !names.insert(o.second.name).second)
					{
						throw std::runtime_error("Duplicate long option name: " + o.second.name);
					}
				}
				for (auto &o : this->longOptions)
				{
					if (o.name.empty())
					{
						throw std::runtime_error("Unnamed long option");
					}
					if (!names.insert(o.name).second)
					{
						throw std::runtime_error("Duplicate long option name: " + o.name);
					}
				}
			}


			// Parse a single option, "next" contains the next item on the command-line (if
			// one exists).
			void ParseOption(int &index, std::string name, std::pair<std::string, bool> *next, bool ignoreUnknowns)
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
					if (!option) for (auto &o : this->longOptions)
					{
						if (name == o.name)
						{
							option = &o;
							break;
						}
					}
				}

				if (option)
				{
					if (!option->set)
					{
						throw std::runtime_error("No variable bound to option: " + name);
					}

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
				else
				{
					if (!ignoreUnknowns) throw std::runtime_error("Unknown option: " + name);
				}
			}


			// Parse a positional argument. If position 0 has been initialised then
			// any arguments that do not match will be added to this.
			void ParsePosition(int pos, std::string value)
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
				else if (this->positions.count(0) && this->positions[0].set)
				{
					// If position 0 has been defined then act as a catch all
					// for all unassigned positional arguments.
					this->positions[0].set(value);
				}
			}


			std::vector<Option> longOptions;
			std::map<char, Option> options;
			std::map<int, Option> positions;
	};
}

