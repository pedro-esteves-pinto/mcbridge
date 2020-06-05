cmake_minimum_required(VERSION 2.8.12)
project(mcbridge)

# helpers to make it safe to use globing to define files in build
include(cmake/safe_dep_glob.cmake)


set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} \
  -Wall \
  -g \
  -pedantic \
  -Wextra \
  --std=c++17\
  -fdiagnostics-color \
  -Werror ")

set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_DEBUG} \
  -DNDEBUG \
  -flto \
  -O3")

if (CMAKE_BUILD_TYPE STREQUAL "" )
  set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS_RELEASE})
endif()

file(GLOB_RECURSE SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*.cpp test/*.cpp)

update_deps_file("${SOURCES}")

add_executable(mcbridge ${SOURCES})
target_include_directories(mcbridge PUBLIC
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/external/asio/asio/include
  )
target_link_libraries(mcbridge pthread)




