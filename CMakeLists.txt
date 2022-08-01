cmake_minimum_required(VERSION 3.0)
project(ePlacePractice)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")

set(DEFLIB_HOME ${PROJECT_SOURCE_DIR}/module/def)
set(LEFLIB_HOME ${PROJECT_SOURCE_DIR}/module/lef)

set(ENV{TYPE_CALL} library)

find_package(Eigen3 REQUIRED)

file(GLOB_RECURSE SRC
        src/DataStructure/Bin.cpp
        src/DataStructure/Circuit.cpp
        src/DataStructure/Parser.cpp
        src/DataStructure/Cell.cpp

        src/Algorithms/FFT.cpp
        src/Algorithms/FFT_calculator.cpp
        )

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


add_executable(ePlacePractice src/main.cpp ${SRC})

target_link_libraries(ePlacePractice def)
target_link_libraries(ePlacePractice lef)
target_link_libraries(ePlacePractice Eigen3::Eigen)
target_include_directories(ePlacePractice PUBLIC ${CMAKE_SOURCE_DIR}/include/DataStructure)
target_include_directories(ePlacePractice PUBLIC ${CMAKE_SOURCE_DIR}/include/Algorithms)
