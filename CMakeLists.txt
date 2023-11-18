cmake_minimum_required(VERSION 3.5)
project(webserver)
add_executable(${PROJECT_NAME} main.cpp)

# options 
find_package(fmt)
target_link_libraries(${PROJECT_NAME} PUBLIC fmt::fmt-header-only)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

