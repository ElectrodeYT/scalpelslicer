cmake_minimum_required(VERSION 3.22)
project(scalpelslicer VERSION 1.0)

include(FetchContent)

find_package(SFML COMPONENTS system window graphics REQUIRED)
find_package(ImGui-SFML REQUIRED)
find_package(Boost REQUIRED)

set(CMAKE_CXX_STANDARD 14)

add_executable(scalpelslicer main.cpp slicer.cpp)
target_include_directories(scalpelslicer PUBLIC .)
target_link_libraries(scalpelslicer sfml-graphics sfml-window sfml-system)
target_link_libraries(scalpelslicer ImGui-SFML::ImGui-SFML)
target_link_libraries(scalpelslicer pthread)
target_link_libraries(scalpelslicer gomp)
target_compile_options(scalpelslicer PRIVATE -fopenmp)