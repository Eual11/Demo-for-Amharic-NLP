cmake_minimum_required(VERSION 3.22)
set(PROJECT_NAME "Learn_Imgui" )

set(PROJECT_DESCRIPTION  "Learning imgui")

project(${PROJECT_NAME} VERSION 1.0 DESCRIPTION "My Project Description" LANGUAGES CXX)
add_subdirectory(vendor)
add_subdirectory(src)
