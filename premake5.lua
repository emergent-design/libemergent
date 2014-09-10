solution "emergent"
	language 		"C++"
	targetdir		"lib"
	includedirs		"include"
	libdirs 		"lib"
	excludes		{ "**.bak", "**~", "**/internal/**" }
	
	configuration "not vs*"
		configurations  { "default" }
		platforms		"native"
	configuration "vs*"
		configurations  { "debug", "release" }
		platforms		{ "x32", "x64" }

	project "libemergent"
		kind 				"SharedLib"
		targetname			"emergent"
		links				{ "freeimage" }
		files 				{ "include/emergent/**.h", "src/emergent/**.cpp" }

		configuration "linux"
			postbuildcommands	{ "./strip lib/libemergent.so" }
			
		configuration "not vs*"
			flags			"Symbols"
			buildoptions	{ "-Wall", "-Wno-sign-compare", "-std=c++11", "-O3", "-D_FORTIFY_SOURCE=2" }
			linkoptions		{ "-Wl,-soname,libemergent.so.0" }

		configuration "vs*"
			kind		"StaticLib"
			defines		"NOMINMAX"
			targetname	"emergent_%{cfg.buildcfg}_%{cfg.platform}"
			
			configuration "debug"
				flags		"Symbols"
			configuration "release"
				optimize	"Full"
