#pragma once

#include <map>
#include <mutex>
#include <memory>
#include <iostream>
#include <typeinfo>
#include <emergent/Emergent.h>


/// The primary method for registering a type, this simply creates an instance
/// of the Type info class for a specific derived class type. To be used in the
/// cpp file for that type.
#define REGISTER_TYPE(base, name) static emergent::Type<base> name##_type(#name, [] { return new name; });


namespace emergent
{
	/// Base class for the type system (to allow a non-templated
	/// concrete implementation to exist in this library that can
	/// contain the static list of masters).
	class TypeBase
	{
		protected:

			/// The global list of type masters. There is a single master for
			/// each base class type used to register other types.
			static std::map<std::string, std::shared_ptr<TypeBase>> masters;

			/// Mutex to help ensure that each type master is only created once.
			static std::mutex cs;
	};


	/// Helper for handling types dynamically, useful when loading classes
	/// from a library and you wish to instantiate them by string name.
	template <class T> class Type : public TypeBase
	{
		public:

			/// Constructor takes the class name, referred to by the lowercase hyphenated
			/// name in the lookup table. It also takes a function which instantiates
			/// the derived type in question.
			Type(std::string name, std::function<T *()> constructor) : constructor(constructor)
			{
				Master()->types[String::hyphenate(name)] = this;
			}


			/// Create an instance of the class from the given name
			/// Returns an empty unique_ptr if the class name cannot be found
			static std::unique_ptr<T> Create(std::string name)
			{
				return Master()->types.count(name)
					? std::unique_ptr<T>(Master()->types[name]->constructor())
					: std::unique_ptr<T>();
			}


			/// Create instances of all registered classes
			static std::map<std::string, std::unique_ptr<T>> CreateAll()
			{
				std::map<std::string, std::unique_ptr<T>> result;

				for (auto &t : Master()->types)
				{
					result.insert(std::make_pair(t.first, std::unique_ptr<T>(t.second->constructor())));
				}

				return result;
			}


			/// Print all registered types derived from T
			/// If multiline is true, each type name is printed on its own line, otherwise
			/// they are printed on a single light with space separation.
			static void Print(bool multiline = true)
			{
				if (!multiline)
				{
					for (auto t : Master()->types) std::cout << t.first << " ";
					std::cout << std::endl;
				}
				else for (auto t : Master()->types) std::cout << "    " << t.first << std::endl;
			}


		private:

			/// Only the master instance is allow to be constructed this way
			Type() {}


			/// Ensures that there is a single master instance for any given
			/// base class type.
			static std::shared_ptr<Type<T>> Master()
			{
				std::lock_guard<std::mutex> lock(cs);

				std::string base = typeid(Type<T>).name();

				if (!masters.count(base)) masters[base] = std::shared_ptr<TypeBase>(new Type<T>());

				return std::static_pointer_cast<Type<T>>(masters[base]);
			}


			/// A function pointer that can be used to create an instance of the class
			std::function<T *()> constructor;


			/// The list of registered types (only accessed in the master)
			std::map<std::string, Type<T>*> types;
	};
}
