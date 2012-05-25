cmake_minimum_required(VERSION 2.0)

project(task-test)

set(LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/obj)

include_directories(${PROJECT_SOURCE_DIR}/../src)

add_library(ok task-ok.c)
target_link_libraries(ok rt)

add_library(notok task-not-ok.c)
target_link_libraries(notok rt)
