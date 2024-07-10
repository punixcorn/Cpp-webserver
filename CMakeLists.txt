cmake_minimum_required(VERSION 3.5)
project(webserver)
add_executable(${PROJECT_NAME} main.cpp)

# options 
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

