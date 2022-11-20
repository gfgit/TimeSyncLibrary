cmake_minimum_required(VERSION 3.5)

project(TimeSyncLibrary LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROGRAM_SOURCES
    main.cpp
    
    src/socketutil.h
    src/socketutil.cpp)



add_executable(TimeSyncLibrary
    ${PROGRAM_SOURCES})

target_include_directories(TimeSyncLibrary PRIVATE src)

if(WIN32)
target_link_libraries(TimeSyncLibrary
    Ws2_32.lib)
endif()
