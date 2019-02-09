cmake_minimum_required(VERSION 3.10)
project(nockets)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (UNIX)
    add_definitions(-DUNIX)
    add_definitions(-DLITTLE_ENDIAN_SYSTEM)
    if("${CMAKE_SYSTEM}" MATCHES "Linux")
        add_definitions(-DLINUX)
    endif()
else()
    add_definitions(-DWIN32)
endif (UNIX)

include_directories(.)
add_subdirectory(../../../util/cpp util)
include_directories(../../../util/cpp ../../../util/cpp/net)

add_library(nockets
	Nocket.cpp
	Nocket.h
	NormUtil.cpp
	NormUtil.h)

set(CMAKE_CXX_FLAGS " -pthread ")

add_executable(nocketrcvr main.cpp)
target_link_libraries(nocketrcvr nockets util)
