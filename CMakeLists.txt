cmake_minimum_required(VERSION 3.20)
project(Spite LANGUAGES C CXX VERSION 0.1.1)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/cmake-deps" CACHE PATH "Shared FetchContent cache" FORCE)

include(cmake/LLVM.cmake)

add_subdirectory(src/runtime)
add_subdirectory(src/compiler)
