#pragma once

#if __has_include(<filesystem>)
	#include <filesystem>
#elif __has_include(<experimental/filesystem>)
	#include <experimental/filesystem>
#endif

namespace emergent
{
	#ifdef __cpp_lib_filesystem
		namespace fs = std::filesystem;
	#elif __cpp_lib_experimental_filesystem
		namespace fs = std::experimental::filesystem;
	#endif
}
