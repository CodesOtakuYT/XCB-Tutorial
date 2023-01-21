cmake_minimum_required(VERSION 3.24)
project(xcb_tutorial C)

set(CMAKE_C_STANDARD 17)

add_executable(xcb_tutorial src/main.c)
target_link_libraries(xcb_tutorial xcb xcb-util xcb-icccm)