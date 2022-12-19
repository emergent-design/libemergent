#pragma once
#include <emergent/FS.hpp>
#include <fstream>


namespace emergent
{
	struct Io
	{
		// Load a whole file into a contiguous buffer type (string/vector)
		template <typename T> static bool Load(T &dst, const fs::path &path)
		{
			static_assert(
				std::is_base_of<
					std::random_access_iterator_tag,
					typename std::iterator_traits<
						decltype(std::begin(std::declval<T const>()))
					>::iterator_category
				>::value,
				"destination must be a contiguous container type"
			);

			static_assert(
				sizeof(typename T::value_type) == 1,
				"destination must be a byte buffer"
			);

			std::ifstream in(path, std::ios::in | std::ios::binary);

			if (in)
			{
				dst.resize(in.seekg(0, std::ios::end).tellg());
				in.seekg(0, std::ios::beg).read((char *)&dst[0], dst.size());

				return true;
			}

			return false;
		}


		template <typename T> static bool Save(const T &src, const fs::path &path)
		{
			static_assert(
				std::is_base_of<
					std::random_access_iterator_tag,
					typename std::iterator_traits<
						decltype(std::begin(std::declval<T const>()))
					>::iterator_category
				>::value,
				"source must be a contiguous container type"
			);

			static_assert(
				sizeof(typename T::value_type) == 1,
				"source must be a byte buffer"
			);

			std::ofstream out(path, std::ios::out | std::ios::binary);

			if (out.good())
			{
				out.write(src.data(), src.size());
				return true;
			}

			return false;
		}
	};
}
