cmake_minimum_required(VERSION 2.0)

project(task-test)

include_directories(${PROJECT_SOURCE_DIR}/../src)

link_directories(${PROJECT_SOURCE_DIR}/../lib)

add_executable(test task-example.c)

target_link_libraries(test cyex)
