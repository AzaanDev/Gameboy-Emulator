project "Gameboy"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	targetdir "bin/%{cfg.buildcfg}"
	staticruntime "on"
	targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
	objdir ("../bin-obj/" .. outputdir .. "/%{prj.name}")

	files { "**.h", "**.cpp" }
	
	includedirs
	{
		"%{IncludeDir.SDL2}",
	}
	
	libdirs {"../vendor/SDL2/SDL2-2.24.1/lib/x64"}
	
	links
	{
		"SDL2.lib",
		"SDL2main.lib"
	}	
	filter "system:windows"
		systemversion "latest"
	
  	filter "configurations:Debug"
		defines { "DEBUG" }
      	symbols "On"
	
   	filter "configurations:Release"
		defines { "NDEBUG" }
      	optimize "On"
		