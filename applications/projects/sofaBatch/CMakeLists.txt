cmake_minimum_required(VERSION 3.22)
project(sofaInfo)

find_package(SofaGeneral)
find_package(SofaMisc)
find_package(SofaExporter)

if(APPLE)
    set(RC_FILES "runSOFA.icns")
else()
    set(RC_FILES "sofa.rc")
endif()

add_executable(${PROJECT_NAME} sofaBatch.cpp)
target_link_libraries(${PROJECT_NAME} SofaGeneral SofaMisc SofaExporter)
if(UNIX)
    target_link_libraries(${PROJECT_NAME} dl)
endif()
