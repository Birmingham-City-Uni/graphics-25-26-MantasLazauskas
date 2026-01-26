cmake_minimum_required(VERSION 3.27.1)

project(Lab1)

add_subdirectory(3rdParty)

include_directories(3rdParty/lodepng)

add_executable(Task1
    Task1.cpp)


target_link_libraries(Task1 
    lodepng
    )

add_executable(Task2
    Task2.cpp)


target_link_libraries(Task2 
    lodepng
    )