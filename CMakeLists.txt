project(nglogger_project)

cmake_minimum_required(VERSION 2.8)
project(nglogger CXX)
set(CMAKE_VERBOSE_MAKEFILE ON)


if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
 set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
 set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a")
 set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wreturn-type -Wno-long-long -pedantic")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
        MESSAGE( ERROR "Intel compiler not supported in our cmake..  Please add" )
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        MESSAGE( ERROR "Microsoft compiler not supported in our cmake..  Please add" )
endif()
MESSAGE(STATUS "CXX FLAGS: ${CMAKE_CXX_FLAGS}")

if (POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif (POLICY CMP0048)


include_directories(SYSTEM ${PROJECT_SOURCE_DIR}/thirdparty/include)
link_directories(${PROJECT_SOURCE_DIR}/thirdparty/lib)

add_subdirectory(nglogger)
add_subdirectory(ngloggerui)
