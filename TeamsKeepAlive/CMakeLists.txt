cmake_minimum_required(VERSION 3.14)
project(TeamsKeepAlive CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT WIN32)
  message(FATAL_ERROR "TeamsKeepAlive is a Windows program. "
                      "Use cmake/mingw-w64-x86_64.cmake to cross-compile.")
endif()

add_executable(TeamsKeepAlive WIN32 src/main.cpp)
target_compile_definitions(TeamsKeepAlive PRIVATE UNICODE _UNICODE)

if(MSVC)
  target_compile_options(TeamsKeepAlive PRIVATE /utf-8)
  # Static CRT so the exe runs without the VC++ redistributable.
  set_property(TARGET TeamsKeepAlive PROPERTY
               MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
  target_compile_options(TeamsKeepAlive PRIVATE -municode)
  # Single self-contained exe: no libstdc++/libgcc/winpthread DLLs needed.
  target_link_options(TeamsKeepAlive PRIVATE -municode -static -s)
endif()

target_link_libraries(TeamsKeepAlive PRIVATE user32 shell32 gdi32)
