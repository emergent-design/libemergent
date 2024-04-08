#pragma once
#include <emergent/Emergent.hpp>
#include <emergent/FS.hpp>
#include <fstream>



namespace emergent
{
	struct Io
	{
		// Load a whole file into a contiguous buffer type (string/vector)
		template <typename T> static bool Load(T &dst, const fs::path &path)
		{
			static_assert(is_contiguous<T>, "destination must be a contiguous container type");
			static_assert(sizeof(typename T::value_type) == 1, "destination must be a byte buffer");

			std::ifstream in(path, std::ios::in | std::ios::binary);

			if (in)
			{
				dst.resize(in.seekg(0, std::ios::end).tellg());
				in.seekg(0, std::ios::beg).read((char *)&dst[0], dst.size());

				return true;
			}

			return false;
		}


		template <typename T> static T Load(const fs::path &path)
		{
			T dst;
			return Load(dst, path) ? dst : T();
		}


		// Save a contiguous buffer to file
		template <typename T> static bool Save(const T &src, const fs::path &path)
		{
			static_assert(is_contiguous<T>, "source must be a contiguous container type");
			static_assert(sizeof(typename T::value_type) == 1, "source must be a byte buffer");

			std::ofstream out(path, std::ios::out | std::ios::binary);

			if (out.good())
			{
				out.write((char *)src.data(), src.size());
				return true;
			}

			return false;
		}
	};
}
