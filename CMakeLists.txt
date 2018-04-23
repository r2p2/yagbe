cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

project(yagbe VERSION 0.0.1 LANGUAGES CXX)

set(DEBUG_CPU "emable cpu debug output" CACHE BOOL OFF)

find_package(SDL2 REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")

set(SOURCE_FILES
  src/main.cc)

add_executable(yagbe
  ${SOURCE_FILES})

if (DEBUG_CPU)
  target_compile_definitions(yagbe PRIVATE -DDEBUG_CPU)
endif()

target_include_directories(yagbe SYSTEM
  PRIVATE ${SDL2_INCLUDE_DIRS})

target_link_libraries(yagbe
  PRIVATE SDL2::SDL2)
