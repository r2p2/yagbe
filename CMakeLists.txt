cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

project(yagbe VERSION 0.0.1 LANGUAGES CXX)

set(DEBUG_CPU "emable cpu debug output" CACHE BOOL OFF)

find_package(SDL2 REQUIRED)
string (STRIP ${SDL2_LIBRARIES} SDL2_LIBRARIES)

find_package(Boost COMPONENTS system thread)

set(CMAKE_CXX_STANDARD 11)
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
  PRIVATE ${SDL2_LIBRARIES})

################################################################################
if (${Boost_FOUND})
  add_executable(yagbe-web
    src/ui-web/main.cc)

  target_include_directories(yagbe-web SYSTEM
    PRIVATE libs/crow
    PRIVATE ${Boost_INCLUDE_DIRS})

  target_link_libraries(yagbe-web
    ${Boost_LIBRARIES})
endif()
