cmake_minimum_required(VERSION 3.20)
project(l2_visualizer CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.91.0
)
FetchContent_MakeAvailable(imgui)

add_library(imgui_obj OBJECT
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer2.cpp
)
target_include_directories(imgui_obj PUBLIC
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
)

find_package(SDL2 REQUIRED)
target_link_libraries(imgui_obj PUBLIC SDL2::SDL2)

add_executable(l2_visualizer
  src/main.cpp
  src/book.hpp
  src/common.hpp
)
target_link_libraries(l2_visualizer PRIVATE imgui_obj SDL2::SDL2)
