cmake_minimum_required(VERSION 3.21)

project(
	Oscilloscope
	VERSION 1.0.0
	DESCRIPTION "A program to draw an oscillogram of incoming sound"
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(
	Oscilloscope 
	src/Oscilloscope.cpp
)

# OpenALUtils
add_subdirectory(external/OpenALUtils)
target_link_libraries(Oscilloscope PRIVATE OpenALUtils)

# SFML
find_package(SFML COMPONENTS system window graphics CONFIG REQUIRED)
target_link_libraries(Oscilloscope PRIVATE sfml-system sfml-network sfml-graphics sfml-window)

# Imgui
find_package(imgui CONFIG REQUIRED)
target_link_libraries(Oscilloscope PRIVATE imgui::imgui)

# Imgui-SFML
find_package(ImGui-SFML CONFIG REQUIRED)
target_link_libraries(Oscilloscope PRIVATE ImGui-SFML::ImGui-SFML)

# Copy resources after build
add_custom_command(
	TARGET Oscilloscope POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_SOURCE_DIR}/resources $<TARGET_FILE_DIR:Oscilloscope>/resources
)