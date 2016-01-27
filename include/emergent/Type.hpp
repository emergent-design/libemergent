#pragma once

#include <map>
#include <mutex>
#include <memory>
#include <iostream>
#include <typeinfo>
#include <emergent/Emergent.hpp>


/// The primary method for registering a type, this simply creates an instance
/// of the Type info class for a specific derived class type. To be used in the
/// cpp file for that type.
#define REGISTER_TYPE(base, name) static emergent::Type<base> name##_type(#name, [] { return new name; });


namespace emergent
{
	/// Helper for handling types dynamically, useful when loading classes
	/// from a library and you wish to instantiate them by string name.
	template <class T> class Type
	{
		public:

			/// Constructor takes the class name, referred to by the lowercase hyphenated
			/// name in the lookup table. It also takes a function which instantiates
			/// the derived type in question.
			Type(std::string name, std::function<T *()> constructor) : constructor(constructor)
			{
				Master().types[String::hyphenate(name)] = this;
			}


			/// Create an instance of the class from the given name
			/// Returns an empty unique_ptr if the class name cannot be found
			static std::unique_ptr<T> Create(std::string name)
			{
				return Master().types.count(name)
					? std::unique_ptr<T>(Master().types[name]->constructor())
					: std::unique_ptr<T>();
			}


			/// Create instances of all registered classes
			static std::map<std::string, std::unique_ptr<T>> CreateAll()
			{
				std::map<std::string, std::unique_ptr<T>> result;

				for (auto &t : Master().types)
				{
					result.insert(std::make_pair(t.first, std::unique_ptr<T>(t.second->constructor())));
				}

				return result;
			}


			/// Print all registered types derived from T
			/// If multiline is true, each type name is printed on its own line, otherwise
			/// they are printed on a single line with space separation.
			static void Print(bool multiline = true)
			{
				if (!multiline)
				{
					for (auto t : Master().types) std::cout << t.first << " ";
					std::cout << std::endl;
				}
				else for (auto t : Master().types) std::cout << "    " << t.first << std::endl;
			}


		private:

			/// Only the master instance is allowed to be constructed this way
			Type() {}


			// Singleton master instance containing the list of registered types
			inline static Type<T> &Master()
			{
				static Type<T> master;
				return master;
			}


			/// A function pointer that can be used to create an instance of the class
			std::function<T *()> constructor;


			/// The list of registered types (only accessed in the master)
			std::map<std::string, Type<T>*> types;
	};
}
