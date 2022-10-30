workspace "Gameboy"
	architecture "x64"
	configurations { "Debug", "Release" }
	startproject "Gameboy"


outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["SDL2"] = "%{wks.location}/vendor/SDL2/SDL2-2.24.1/include"


include "Gameboy"