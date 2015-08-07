solution "emergent"
	language 		"C++"
	flags			"Symbols"
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
		files				{ "src/test/**.cpp" }
		postbuildcommands	{ "./bin/test" }
