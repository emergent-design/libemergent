#pragma once


namespace emergent
{
	/// Template for creating a singleton from a class
	template <class T> class Singleton
	{
		public:
			/// Get the singleton instance
			static T& Instance()
			{
				static T _instance;
				return _instance;
			}

		private:
			Singleton();							///< Prevent instantiation
			Singleton(Singleton const&);			///< Prevent copy construction
			Singleton& operator=(Singleton const&);	///< Prevent assignment
			~Singleton();							///< Prevent destruction
	};
}