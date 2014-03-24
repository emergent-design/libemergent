#pragma once

#include <string>
#include <vector>


namespace emergent
{
	/// A helper class for dealing with paths. It is based on the boost implementation
	/// although it only contains the Linux backend at this time and is therefore the
	/// only part of the libemergent that is not entirely cross-platform yet.
	struct Path
	{
		/// Default, copy and string constructors
		Path() {}
		Path(const Path &p)			: path(p.path)	{}
		Path(const std::string &s)	: path(s)		{}
		Path(const char *s)			: path(s)		{}

		/// Implicit string conversion
		operator std::string() const { return this->path; }

		/// Path concatenation operators
		Path &operator/=(const Path &p);
		Path operator/(const Path &p) const;

		/// Returns the file extension. If no extension exists for the
		/// the current path then it returns an empty string.
		std::string extension() const;

		/// Returns just the filename, so anything after the final
		/// separator or else an empty string
		std::string filename() const;

		/// Returns the filename without the extension
		std::string stem() const;

		/// Returns the parent of this path, if the current path
		/// is a file then it is the containing directory else
		/// it is the directory above.
		Path parent() const;

		/// Returns true if the internal path is empty
		bool empty() const;

		/// Checks if the file or directory exists
		bool exists() const;

		/// Returns true if this path is a file
		bool file() const;

		/// Returns true if this path is a directory
		bool directory() const;

		/// Create the directory given by the path
		static bool create_directory(const Path &path);

		/// Recursively create any directories in the path that do no exist
		static bool create_directories(const Path &path);

		/// Delete the file given by the path
		static bool delete_file(const Path &path);

		/// Delete the directory given by the path
		static bool delete_directory(const Path &path);

		/// Returns a list of children (files and directories) of the
		/// the current path. If the supplied flag is true then the
		/// list is sorted using a quicksort.
		std::vector<Path> children(bool sorted = true) const;

		private:

			/// Adds a separator to the path if it is not empty
			/// and does not already end in a separator
			void append_separator();

			/// The actual path
			std::string path;
	};


	/// Helper so that a path can be passed to cout
	std::ostream& operator<<(std::ostream &output, const Path &p);
}
