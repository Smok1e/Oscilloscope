cmake_minimum_required(VERSION 3.21)

# Ensure that the repo is cloned properly
if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/OpenALUtils/CMakeLists.txt")
	message(FATAL_ERROR "external/OpenALUtils does not contains CMakeLists.txt; Perhaps you forget to do git submodule init && git submodule update?")
endif ()

project(
	Oscilloscope
	VERSION 1.0.0
	DESCRIPTION "A program to draw an oscillogram of incoming sound"
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(
	Oscilloscope 
	src/Oscilloscope.cpp
	src/ImmersiveDarkMode.cpp
)

# ImGUI-SFML
if (WIN32)
	# On windows use vcpkg
	find_package(ImGui-SFML CONFIG REQUIRED)             
	target_link_libraries(Oscilloscope PRIVATE ImGui-SFML::ImGui-SFML) 

	add_compile_definitions(OSCILLOSCOPE_WINDOWS)
else ()
	# Otherwise building from sources
	add_subdirectory(external/ImGUI-SFML)
	target_link_libraries(Oscilloscope PRIVATE ImGui-SFML::ImGui-SFML)
endif ()

# OpenALUtils
add_subdirectory(external/OpenALUtils)
target_link_libraries(Oscilloscope PRIVATE OpenALUtils)

# Copy resources after build
add_custom_command(
	TARGET Oscilloscope POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_SOURCE_DIR}/resources $<TARGET_FILE_DIR:Oscilloscope>/resources
)
