workspace "Minesweeper"
    architecture "x64"
    startproject "Minesweeper"
    configurations { "Debug", "Release" }

    project "Minesweeper"
        location "%{wks.location}"
        kind "ConsoleApp"
        language "C"
        cdialect "C99"

        targetdir ("%{wks.location}/bin/")
        objdir ("%{wks.location}/bin-int/")
        targetname("%{prj.name}-%{cfg.buildcfg}")

        includedirs "%{wks.location}/SDL/include/"

        files "src/**"

        staticruntime "off"
        systemversion "latest"

        filter "system:linux"
            links { "SDL2", "m" }

        filter "system:windows"
            links "%{wks.location}/SDL/Win64/SDL2.lib"

        filter "configurations:Debug"
            symbols "on"
            runtime "Debug"

        filter "configurations:Release"
            runtime "Release"
            optimize "on"
