#pragma once

#if __has_include(<filesystem>)
	#include <filesystem>
#elif __has_include(<experimental/filesystem>)
	#include <experimental/filesystem>
#endif
#include <chrono>

namespace emergent
{
	#ifdef __cpp_lib_filesystem
		namespace fs = std::filesystem;
	#elif __cpp_lib_experimental_filesystem
		namespace fs = std::experimental::filesystem;
	#endif

	namespace time
	{
		// Convert from std::filesystem::file_time_type to a time_t
		// Possible precision issues but good enough for general use
		inline time_t to_time_t(const fs::file_time_type &time)
		{
			// clock_cast does not actually appear to be available in C++20 at this time
			// #if __cplusplus > 201703L
			// 	return std::chrono::system_clock::to_time_t(
			// 		std::chrono::clock_cast<std::chrono::system_clock>(time)
			// 	);
			// #else
			return std::chrono::system_clock::to_time_t(
				std::chrono::time_point_cast<std::chrono::system_clock::duration>(
					time - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
				)
			);
			// #endif
		}
	}
}
