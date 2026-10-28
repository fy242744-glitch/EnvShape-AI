cmake_minimum_required(VERSION 3.16)
project(EnvShapeAI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/include)

# 查找 CURL 库
find_package(CURL REQUIRED)

add_executable(envshape src/main.cpp)

# 链接 CURL 库
target_link_libraries(envshape PRIVATE CURL::libcurl)

if (MSVC)
    target_compile_options(envshape PRIVATE /W4)
else()
    target_compile_options(envshape PRIVATE -Wall -Wextra)
endif()