solution "emergent"
	language 		"C++"
	targetdir		"lib"
	flags 			"Symbols"
	configurations	"default"
	platforms		"native"
	buildoptions	{ "-Wall", "-Wno-sign-compare", "-std=c++11", "-O3", "-fPIC", "-D_FORTIFY_SOURCE=2" }
	includedirs		{ "include" }
	libdirs 		{ "lib" }
	excludes		{ "**.bak", "**~", "**/internal/**" }

	project "libemergent"
		kind 				"SharedLib"
		targetname			"emergent"
		links				{ "freeimageplus" }
		linkoptions			"-Wl,-soname,libemergent.so.0"
		includedirs			{ "include/emergent" }
		files 				{ "include/emergent/**.h", "src/emergent/**.cpp" }
		postbuildcommands	{ "./strip lib/libemergent.so" }
