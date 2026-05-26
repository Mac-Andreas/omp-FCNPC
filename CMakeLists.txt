cmake_minimum_required(VERSION 3.19)
project(FCNPC LANGUAGES C CXX VERSION 2.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# open.mp SDK (header-only INTERFACE target; pulls vendored libs from sdk/lib).
add_subdirectory(sdk)

include_directories(
	.
	src
	pawn/source
	pawn/source/amx
	pawn/source/linux
)

add_definitions(
	-DHAVE_STDINT_H=1
	-DPAWN_CELL_SIZE=32
)

add_library(${PROJECT_NAME} SHARED
	src/main.cpp
	src/FCNPC.cpp
	src/natives.cpp
	src/natives_array.cpp
	src/bridge.cpp
)

# Components are loaded by exact file name; drop the "lib" prefix on *nix.
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")

target_link_libraries(${PROJECT_NAME} PRIVATE
	OMP-SDK
)
