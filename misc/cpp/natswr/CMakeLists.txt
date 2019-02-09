cmake_minimum_required(VERSION 2.8)
macro(use_cxx11)
  if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      add_compile_options(-std=c++11)
      #add_compile_options(-std=gnu++11)
    else ()
      add_compile_options(-std=c++11)
    endif ()
  else ()
    set (CMAKE_CXX_STANDARD 11)
  endif ()
endmacro(use_cxx11)

use_cxx11()

project(netwr)

if (WIN32)
    add_definitions(-DWIN32)
elseif(UNIX)
    add_definitions(-DUNIX)
endif (WIN32)

set(CMAKE_CXX_FLAGS -pthread)
set(SOURCE_FILES NatsWrapper.cpp)
set(HEADER_FILES NatsWrapper.h)

#------------------------------------------------

# cnats
add_subdirectory(${PROJECT_SOURCE_DIR}/../../../externals/cnats/cnats-master/
        ${PROJECT_SOURCE_DIR}/.../../../externals/cnats/cnats-master/install/lib/)
include_directories(${PROJECT_SOURCE_DIR}/../../../externals/cnats/cnats-master/src)
link_directories(${PROJECT_SOURCE_DIR}/../../../externals/cnats/cnats-master/install/lib)

# util (this does not build util... I could do it, but since we will switch to cmake, I don't want to bother...
include_directories(${PROJECT_SOURCE_DIR}/../../../util/cpp)
link_directories(${PROJECT_SOURCE_DIR}/../../../util/cpp/linux)

#------------------------------------------------

add_library(natswr STATIC ${SOURCE_FILES} ${HEADER_FILES})
add_executable(natswr_pub NatsPublisher.cpp)
add_executable(natswr_sub NatsSubscriber.cpp)

target_link_libraries(natswr_pub natswr libutil.a libsecurity.a libnats_static.a)
target_link_libraries(natswr_sub natswr libutil.a libsecurity.a libnats_static.a)
