workspace "Enigma"
	language "C++"
	cppdialect "C++20"
	location ""
	platforms { "x64" }
	configurations { "Debug", "Release" }


	startproject "Enigma"
	links{"libs/vulkan/vulkan-1"}	
dofile( "libs/glslc.lua" )

project "Enigma"
	local sources = { 
		"src/Core/**.cpp",
		"src/Core/**.h",
		"src/Graphics/**.h",
        "src/Graphics/**.cpp",
		"src/**.cpp",
		"libs/imgui/**.cpp",
		"libs/imgui/**.h"
	}

	kind "ConsoleApp"
	location "Enigma"

	files( sources )

	includedirs {
		"/libs/GLFW/include/",
		"libs/",
		"libs/glm/",
		"libs/include/",
		"libs/include/",
		"libs/Volk/",
		"libs/vulkan/include/",
		"libs/VulkanMemoryAllocator/include/",
		"libs/rapidobj/",
		"libs/stb/",
		"libs/assimp_x64-windows/",
		"libs/assimp_x64-windows/include/",
		"libs/imgui"
	}

	links {
		"libs/vulkan/vulkan-1.lib",
		"libs/vulkan/volk.lib",
		"libs/GLFW/lib/glfw3.lib",
		"libs/assimp_x64-windows/lib/assimp-vc143-mt.lib"
	}

	dependson "Enigma-shaders"

	postbuildcommands { 
		"{COPY} \"$(SolutionDir)assimp-vc143-mt.dll\" \"$(OutDir)\"",


		"{COPY} \"$(SolutionDir)minizip.dll\" \"$(OutDir)\"",
		"{COPY} \"$(SolutionDir)poly2tri.dll\" \"$(OutDir)\"",
		"{COPY} \"$(SolutionDir)pugixml.dll\" \"$(OutDir)\"",
		"{COPY} \"$(SolutionDir)zlib1.dll\" \"$(OutDir)\"",

	}

project "Enigma-shaders"

	kind "Utility"
	location ""
	targetdir "resources/Shaders/"
	objdir "resources/Shaders/"
    files { 
        "resources/Shaders/*.vert",
        "resources/Shaders/*.frag",
        "resources/Shaders/*.comp",
        "resources/Shaders/*.geom",
        "resources/Shaders/*.tesc",
        "resources/Shaders/*.tese"
    }


	files( shaders )

	handle_glsl_files( "-O", "Enigma/resources/Shaders", {} )
