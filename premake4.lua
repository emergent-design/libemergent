solution "emergent"
	language 		"C++"
	targetdir		"lib"
	configurations	"default"
	platforms		"native"
	includedirs		{ "include" }
	libdirs 		{ "lib" }
	excludes		{ "**.bak", "**~", "**/internal/**" }

	project "libemergent"
		kind 				"SharedLib"
		targetname			"emergent"
		links				{ "freeimage" }
		includedirs			{ "include/emergent" }
		files 				{ "include/emergent/**.h", "src/emergent/**.cpp" }

		configuration "not vs*"
			flags				"Symbols"
			buildoptions		{ "-Wall", "-Wno-sign-compare", "-std=c++11", "-O3", "-D_FORTIFY_SOURCE=2" }
			linkoptions			{ "-Wl,-soname,libemergent.so.0" }

		configuration "linux"
			postbuildcommands	{ "./strip lib/libemergent.so" }

		configuration "vs*"
			kind "StaticLib"
