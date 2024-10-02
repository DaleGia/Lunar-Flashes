cmake_minimum_required(VERSION 3.0)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif() 

project(LunarFlashes LANGUAGES CXX)


# add hardcoded guesses for the location of Vmb to CMAKE_PREFIX_PATH
list(APPEND CMAKE_PREFIX_PATH "/usr/local/lib/VimbaX/api")
list(APPEND CMAKE_PREFIX_PATH "/usr/local/lib/VimbaX/api/include/VmbCPP")
list(APPEND CMAKE_PREFIX_PATH "/usr/local/lib/VimbaX/api/include/VmbImageTransform")
list(APPEND CMAKE_PREFIX_PATH "/usr/local/lib/VimbaX/lib")
list(APPEND CMAKE_PREFIX_PATH "/usr/local/lib/VimbaX/lib/cmake")

if(EXISTS "/usr/local/lib/VimbaX/cmake/vmb_cmake_prefix_paths.cmake")
    # read hardcoded package location information, if the example is still located in the original install location
    include("/usr/local/lib/VimbaX/cmake/vmb_cmake_prefix_paths.cmake")
endif()


if(NOT WIN32)
    list(APPEND CMAKE_EXE_LINKER_FLAGS "-Wl,-rpath,'\$ORIGIN'")
endif()

add_executable(LunarFlashes main.cpp)

############# IMAGE MAGICK STUFF #################################
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wfatal-errors")
add_definitions( -DMAGICKCORE_QUANTUM_DEPTH=16 )
add_definitions( -DMAGICKCORE_HDRI_ENABLE=0 )
find_package(ImageMagick COMPONENTS Magick++)
include_directories(${ImageMagick_INCLUDE_DIRS})
target_link_libraries(LunarFlashes PRIVATE ${ImageMagick_LIBRARIES})
##################################################################

############# MOSQUITTO STUFF #######################################
find_package(PkgConfig REQUIRED)
pkg_check_modules(Mosquitto IMPORTED_TARGET libmosquitto REQUIRED)
target_link_libraries(LunarFlashes PRIVATE mosquitto)
##################################################################
find_package(Vmb REQUIRED COMPONENTS CPP)
find_package(Vmb REQUIRED COMPONENTS ImageTransform)
target_link_libraries(LunarFlashes PRIVATE Vmb::CPP)
target_link_libraries(LunarFlashes PRIVATE Vmb::ImageTransform)
set_target_properties(LunarFlashes PROPERTIES
    CXX_STANDARD 11
    VS_DEBUGGER_ENVIRONMENT "PATH=${VMB_BINARY_DIRS};$ENV{PATH}")
