-- premake5.lua
workspace "text-editor"
   configurations { "Debug", "Release" }

project "text-editor"
   kind "WindowedApp"
   language "C++"
   targetdir "bin/%{cfg.buildcfg}"
   links { "Dwmapi" }

   files { "**.h", "**.cpp", "**.c" }

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"