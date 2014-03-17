#pragma once

#include <string>
#include <vector>


namespace emergent
{
	struct Path
	{
		Path() {}
		Path(const Path &p)			: path(p.path)	{}
		Path(const std::string &s)	: path(s)		{}
		Path(const char *s)			: path(s)		{}

		operator std::string() const { return this->path; }

		Path &operator/=(const Path &p);
		Path operator/(const Path &p) const;

		std::string extension() const;
		std::string filename() const;
		std::string stem() const; 		// Filename without extension

		Path parent() const;

		bool empty() const;
		bool exists() const;
		bool file() const;
		bool directory() const;

		static bool create_directory(const Path &path);
		static bool create_directories(const Path &path);

		static bool delete_file(const Path &path);
		static bool delete_directory(const Path &path);

		std::vector<Path> children(bool sorted = true) const;

		private:

			void append_separator();	// If necessary
			std::string path;
	};

	std::ostream& operator<<(std::ostream &output, const Path &p);
}
