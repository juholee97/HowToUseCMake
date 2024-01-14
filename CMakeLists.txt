cmake_minimum_required(VERSION 3.14)
project(learn_cmake_with_gtest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CSS_STANDARD_REQUIRED_ON)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

file(GLOB_RECURSE PROJECT_SOURCE_FILE
	./src/*.hpp 
	./src/*.cpp
)

add_executable(learn_cmake_with_gtest ${PROJECT_SOURCE_FILE})
target_link_libraries(learn_cmake_with_gtest gtest_main)