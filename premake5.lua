solution "emergent"
	language 		"C++"
	symbols			"On"
	configurations	"default"
	platforms		"native"
	toolset			"clang"
	includedirs		"include"
	buildoptions	{ "-Wall", "-Wno-sign-compare", "-std=c++14" }
	excludes		{ "**.bak", "**~", "**/internal/**" }

	project "test"
		kind				"ConsoleApp"
		targetdir			"bin"
		targetname			"test"
		links				"freeimage"
		files				{ "src/test/**.cpp" }
		postbuildcommands	{ "./bin/test" }
