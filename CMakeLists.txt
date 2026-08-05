cmake_minimum_required(VERSION 4.2)
project(libtui)

set(CMAKE_CXX_STANDARD 20)

add_library(libtui INTERFACE
        snapcell.hpp)

#target_include_directories(libtui INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(test_run main.cpp)

target_link_libraries(test_run PRIVATE libtui)

# 3. Include directories for WebP headers if necessary
target_include_directories(test_run PRIVATE ${WEBP_INCLUDE_DIRS})

# 4. Link the webp library to resolve 'WebPDecodeRGBA'
target_link_libraries(test_run PRIVATE ${WEBP_LIBRARIES})