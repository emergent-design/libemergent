#pragma once

#include <map>
#include <mutex>
#include <memory>
#include <typeinfo>
#include <emergent/Emergent.h>

#define REGISTER_TYPE(base, name) emergent::Type<base> name##_type(#name, [] { return new name; });


namespace emergent
{
	/// Base class for the type system (to allow a non-templated
	/// concrete implementation to exist in this library that can
	/// contain the static list of masters).
	class TypeBase
	{
		protected:
			
			static std::map<std::string, std::shared_ptr<TypeBase>> masters;
			static std::mutex cs;
	};


	/// Helper for handling types dynamically, useful when loading classes 
	/// from a library and you wish to instantiate them by string name.
	template <class T> class Type : public TypeBase
	{
		public:

			Type(std::string name, std::function<T *()> constructor) : constructor(constructor)
			{
				Master()->types[hyphenate(name)] = this;
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

		
		private:
			
			/// Only the master instance is allow to be constructed this way
			Type() {}

			/// Ensures that there is a single master instance
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
