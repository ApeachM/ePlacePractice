cmake_minimum_required(VERSION 3.0)
project(ePlacePractice)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")

set(DEFLIB_HOME ${PROJECT_SOURCE_DIR}/module/def)
set(LEFLIB_HOME ${PROJECT_SOURCE_DIR}/module/lef)

set(ENV{TYPE_CALL} library)

file(GLOB_RECURSE DataStructure
        src/DataStructure/Circuit.cpp
        src/DataStructure/Parser.cpp)

add_subdirectory(${DEFLIB_HOME})
add_subdirectory(${LEFLIB_HOME})

############################################################
# include directories
############################################################
include_directories(
        ${DEFLIB_HOME}/def
        ${DEFLIB_HOME}/defzlib
        ${DEFLIB_HOME}/cdef
        ${DEFLIB_HOME}/cdefzlib

        ${LEFLIB_HOME}/lef
        ${LEFLIB_HOME}/lefzlib
        ${LEFLIB_HOME}/clef
        ${LEFLIB_HOME}/clefzlib
)


add_executable(ePlacePractice src/main.cpp ${DataStructure})

target_link_libraries(ePlacePractice def)
target_link_libraries(ePlacePractice lef)
target_include_directories(ePlacePractice PUBLIC ${CMAKE_SOURCE_DIR}/include/DataStructure)
