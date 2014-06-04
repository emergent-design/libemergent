#include "emergent/Path.h"
#include <algorithm>

#ifdef __linux
	#include <sys/stat.h>
	#include <unistd.h>
	#include <dirent.h>

	const char SEPARATOR = '/';
#endif

#ifdef _WIN32
	const char SEPARATOR = '\\';
#endif

using namespace std;


namespace emergent
{
	ostream& operator<<(ostream &output, const Path &p) { return output << (string)p; }


	Path &Path::operator/=(const Path &p)
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


	Path Path::operator/(const Path &p) const
	{
		return Path(*this) /= p;
	}


	void Path::append_separator()
	{
		if (!this->path.empty())
		{
			if (*this->path.rbegin() != SEPARATOR)
			{
				this->path += SEPARATOR;
			}
		}
	}


	string Path::extension() const
	{
		string name = this->filename();
		auto pos	= name.rfind('.');
		return pos == string::npos ? "" : name.substr(pos+1);
	}


	string Path::filename() const
	{
		if (!this->path.empty())
		{
			if (*this->path.rbegin() != SEPARATOR)
			{
				auto pos	= this->path.find_last_of(SEPARATOR);
				return pos == string::npos ? this->path : this->path.substr(pos+1);
			}
		}

		return "";
	}


	string Path::stem() const
	{
		string name = this->filename();
		auto pos	= name.rfind('.');
		return pos == string::npos ? name : name.substr(0, pos);
	}


	Path Path::parent() const
	{
		if (!this->path.empty() && this->path != "/")
		{
			// Skip the final character in case it is a trailing '/'
			auto pos	= this->path.find_last_of(SEPARATOR, this->path.size() - 2);	
			return pos == 0 ? Path("/") : pos == string::npos ? Path() : Path(this->path.substr(0, pos));
		}

		return Path();
	}


	bool Path::empty() const
	{
		return this->path.empty();
	}


	#ifdef __linux
		bool Path::exists() const
		{
			struct stat info;
			return stat(this->path.c_str(), &info) == 0;
		}


		bool Path::file() const
		{
			struct stat info;
			return stat(this->path.c_str(), &info) == 0 ? S_ISREG(info.st_mode) : false;
		}


		bool Path::directory() const
		{
			struct stat info;
			return stat(this->path.c_str(), &info) == 0 ? S_ISDIR(info.st_mode) : false;
		}


		vector<Path> Path::children(bool sorted) const
		{
			vector<Path> result;

			if (this->directory())
			{
				struct dirent *entry;
				DIR  *dir = opendir(this->path.c_str());

				if (dir)
				{
					while ((entry = readdir(dir)))
					{
						string name = entry->d_name;

						if (name != "." && name != "..") result.push_back(*this / name);
					}
					closedir(dir);
				}

				if (sorted) sort(result.begin(), result.end(), [](const Path &a, const Path &b) { return a.path < b.path; });
			}

			return result;
		}


		bool Path::create_directory(const Path &path)
		{
			return ::mkdir(path.path.c_str(), S_IRWXU|S_IRWXG|S_IRWXO) == 0;
		}


		bool Path::create_directories(const Path &path)
		{
			if (!path.exists())
			{
				Path parent = path.parent();

				if (!parent.empty() && !parent.exists()) create_directories(parent);

				return create_directory(path);
			}

			return false;
		}


		bool Path::delete_file(const Path &path)
		{
			return ::unlink(path.path.c_str()) == 0;
		}


		bool Path::delete_directory(const Path &path)
		{
			return ::rmdir(path.path.c_str()) == 0;
		}

	#endif
}