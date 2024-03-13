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
		"src/**.cpp"
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
		"libs/stb/"
	}

	links {
		"libs/vulkan/vulkan-1.lib",
		"libs/vulkan/volk.lib",
		"libs/GLFW/lib/glfw3.lib"
	}

	dependson "Enigma-shaders"

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