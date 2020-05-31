cmake_minimum_required(VERSION 2.8.12)
project(mcbridge)

# helpers to make it safe to use globing to define files in build
include(cmake/safe_dep_glob.cmake)

set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} \
  -Wall \
  -pedantic \
  -Wno-format-security \
  -Wextra \
  -std=c++2a\
  -Werror ")

set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_DEBUG} \
  -DNDEBUG \
  -flto \
  -O3")

file(GLOB_RECURSE SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*.cpp)

update_deps_file("${SOURCES}")

add_executable(mcbridge ${SOURCES})
target_include_directories(mcbridge PUBLIC ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/external/asio/asio/include)
target_link_libraries(mcbridge pthread)




