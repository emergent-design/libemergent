solution "emergent"
	language 		"C++"
	symbols			"On"
	configurations	"default"
	platforms		"native"
	toolset			"clang"
	includedirs		"include"
	-- buildoptions	{ "-Wall", "-Wextra", "-Wpedantic", "-Wno-sign-compare", "-std=c++17" }
	buildoptions	{ "-Wall", "-Wextra", "-Wpedantic", "-std=c++17" }
	excludes		{ "**.bak", "**~", "**/internal/**" }

	project "test"
		kind				"ConsoleApp"
		targetdir			"bin"
		targetname			"test"
		links				"freeimage"
		files				{ "src/test/**.cpp" }
		postbuildcommands	{ "./bin/test" }
