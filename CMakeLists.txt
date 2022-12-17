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

        src/Utility/Visualizer.cpp
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
target_include_directories(ePlacePractice PUBLIC ${CMAKE_SOURCE_DIR}/include/Utility)
target_include_directories(ePlacePractice PUBLIC ${CMAKE_SOURCE_DIR}/module/CImg)
target_link_libraries(ePlacePractice X11 pthread)


#set(YOU_NEED_CIMG 1)
#if (YOU_NEED_CIMG)
#    target_include_directories(ePlacePractice PUBLIC ${CMAKE_SOURCE_DIR}/module/CImg)
#    message(STATUS "Looking for Threads...")
#    find_package(Threads REQUIRED)
#    target_link_libraries(ePlacePractice Threads::Threads)
#    message(STATUS "Looking for X11...")
#    find_package(X11 REQUIRED)
#    include_directories(${X11_INCLUDE_DIR})
#    target_link_libraries(ePlacePractice ${X11_LIBRARIES})
#    message(STATUS "Looking for libjpg...")
#    find_package(JPEG REQUIRED)
#    include_directories(${JPEG_INCLUDE_DIR})
#    target_link_libraries (ePlacePractice ${JPEG_LIBRARY})
#    target_compile_definitions(ePlacePractice PRIVATE cimg_use_jpeg=1)
#    message(STATUS "Looking for libpng...")
#    find_package(PNG REQUIRED)
#    include_directories(${PNG_INCLUDE_DIR})
#    target_link_libraries (ePlacePractice ${PNG_LIBRARY})
#    target_compile_definitions(ePlacePractice PRIVATE cimg_use_png=1)
#endif ()
#
