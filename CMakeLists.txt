project(sicxe CXX)
cmake_minimum_required(VERSION 2.8)

# compiler settings
set(CMAKE_CXX_FLAGS          "${CMAKE_CXX_FLAGS} -Wall -Wshadow -Wextra -Werror")
set(CMAKE_CXX_FLAGS          "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_FLAGS          "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti")
set(CMAKE_CXX_FLAGS_DEBUG    "-O0")
set(CMAKE_CXX_FLAGS_RELEASE  "-Os -DNDEBUG")

# include header files from the src root
set(CMAKE_CXX_FLAGS          "${CMAKE_CXX_FLAGS} -iquote ${PROJECT_SOURCE_DIR}/src")

add_subdirectory(src)
add_subdirectory(tests)
