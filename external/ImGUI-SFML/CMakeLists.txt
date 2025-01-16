cmake_minimum_required(VERSION 3.21)
cmake_policy(SET CMP0135 OLD)

include(FetchContent)

set(SFML_VERSION 2.6.2)
set(IMGUI_VERSION 1.91.7)

FetchContent_Declare(
	SFML
	URL "https://github.com/SFML/SFML/archive/${SFML_VERSION}.zip"
)

FetchContent_Declare(
	imgui
	URL "https://github.com/ocornut/imgui/archive/v${IMGUI_VERSION}.zip"
)

FetchContent_Declare(
	imgui-sfml
	GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
	GIT_TAG        2.6.x
)   

option(SFML_BUILD_AUDIO "Build audio" OFF)
option(SFML_BUILD_NETWORK "Build network" OFF)
FetchContent_MakeAvailable(sfml imgui)

set(IMGUI_DIR ${imgui_SOURCE_DIR})
option(IMGUI_SFML_FIND_SFML "Use find_package to find SFML" OFF)
FetchContent_MakeAvailable(imgui-sfml)
