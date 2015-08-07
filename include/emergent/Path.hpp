#pragma once

#include <string>
#include <vector>
#include <algorithm>

#ifdef __linux
	#include <sys/stat.h>
	#include <unistd.h>
	#include <dirent.h>
#endif

#ifdef _WIN32
#endif


namespace emergent
{
	/// A helper class for dealing with paths. It is based on the boost implementation
	/// although it only contains the Linux backend at this time and is therefore the
	/// only part of the libemergent that is not entirely cross-platform yet.
	struct Path
	{
		#ifdef __linux
			static const char SEPARATOR = '/';
		#endif
		#ifdef _WIN32
			static const char SEPARATOR = '\\';
		#endif


		/// Default, copy and string constructors
		Path() {}
		Path(const Path &p)			: path(p.path)	{}
		Path(const std::string &s)	: path(s)		{}
		Path(const char *s)			: path(s)		{}


		/// Implicit string conversion
		operator std::string() const { return this->path; }


		/// Path concatenation operator
		Path &operator/=(const Path &p)
		{
			if (!p.empty())
			{
				if (p.path[0] != SEPARATOR)
				{
					this->append_separator();
				}

				this->path += p.path;
			}

			return *this;
		}


		/// Path concatenation operator
		Path operator/(const Path &p) const
		{
			return Path(*this) /= p;
		}


		/// Returns the file extension. If no extension exists for the
		/// the current path then it returns an empty string.
		std::string extension() const
		{
			auto name	= this->filename();
			auto pos	= name.rfind('.');

			return pos == std::string::npos ? "" : name.substr(pos+1);
		}

		/// Returns just the filename, so anything after the final
		/// separator or else an empty string
		std::string filename() const
		{
			if (!this->path.empty())
			{
				if (*this->path.rbegin() != SEPARATOR)
				{
					auto pos	= this->path.find_last_of(SEPARATOR);
					return pos == std::string::npos ? this->path : this->path.substr(pos+1);
				}
			}

			return "";
		}


		/// Returns the filename without the extension
		std::string stem() const
		{
			auto name 	= this->filename();
			auto pos	= name.rfind('.');

			return pos == std::string::npos ? name : name.substr(0, pos);
		}


		/// Returns the parent of this path, if the current path
		/// is a file then it is the containing directory else
		/// it is the directory above.
		Path parent() const
		{
			if (!this->path.empty() && this->path != "/")
			{
				// Skip the final character in case it is a trailing '/'
				auto pos	= this->path.find_last_of(SEPARATOR, this->path.size() - 2);
				return pos == 0 ? Path("/") : pos == std::string::npos ? Path() : Path(this->path.substr(0, pos));
			}

			return Path();
		}


		/// Returns true if the internal path is empty
		bool empty() const
		{
			return this->path.empty();
		}


	#ifdef __linux

		/// Checks if the file or directory exists
		bool exists() const
		{
			struct stat info;
			return stat(this->path.c_str(), &info) == 0;
		}


		/// Returns true if this path is a file
		bool file() const
		{
			struct stat info;
			return stat(this->path.c_str(), &info) == 0 ? S_ISREG(info.st_mode) : false;
		}


		/// Returns true if this path is a directory
		bool directory() const
		{
			struct stat info;
			return stat(this->path.c_str(), &info) == 0 ? S_ISDIR(info.st_mode) : false;
		}


		/// Create the directory given by the path
		static bool create_directory(const Path &path)
		{
			return ::mkdir(path.path.c_str(), S_IRWXU|S_IRWXG|S_IRWXO) == 0;
		}


		/// Recursively create any directories in the path that do no exist
		static bool create_directories(const Path &path)
		{
			if (!path.exists())
			{
				Path parent = path.parent();

				if (!parent.empty() && !parent.exists()) create_directories(parent);

				return create_directory(path);
			}

			return false;
		}


		/// Delete the file given by the path
		static bool delete_file(const Path &path)
		{
			return ::unlink(path.path.c_str()) == 0;
		}


		/// Delete the directory given by the path
		static bool delete_directory(const Path &path)
		{
			return ::rmdir(path.path.c_str()) == 0;
		}


		/// Returns a list of children (files and directories) of the
		/// the current path. If the supplied flag is true then the
		/// list is sorted using a quicksort.
		std::vector<Path> children(bool sorted = true) const
		{
			std::vector<Path> result;

			if (this->directory())
			{
				struct dirent *entry;
				DIR  *dir = opendir(this->path.c_str());

				if (dir)
				{
					while ((entry = readdir(dir)))
					{
						std::string name = entry->d_name;

						if (name != "." && name != "..") result.push_back(*this / name);
					}
					closedir(dir);
				}

				if (sorted)
				{
					std::sort(
						result.begin(), result.end(),
						[](const Path &a, const Path &b) { return a.path < b.path; }
					);
				}
			}

			return result;
		}

	#endif

		private:
			/// Adds a separator to the path if it is not empty
			/// and does not already end in a separator
			void append_separator()
			{
				if (!this->path.empty())
				{
					if (*this->path.rbegin() != SEPARATOR)
					{
						this->path += SEPARATOR;
					}
				}
			}

			/// The actual path
			std::string path;
	};


	/// Helper so that a path can be passed to cout
	inline std::ostream& operator<<(std::ostream &output, const Path &p)
	{
		return output << (std::string)p;
	}
}

