cmake_minimum_required(VERSION 3.20) # VERSION 4.2 este invalid/viitor; folosește o versiune standard (ex. 3.20)
project(libtui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Căutăm pachetul oficial WebP
find_package(WebP REQUIRED)

add_library(libtui INTERFACE)
target_sources(libtui INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/snapcell.hpp)
target_include_directories(libtui INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(test_run main.cpp)

# 2. Linkăm libtui și ținta WebP găsită de CMake
target_link_libraries(test_run PRIVATE
        libtui
        WebP::webp
)