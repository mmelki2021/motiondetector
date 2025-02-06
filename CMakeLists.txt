cmake_minimum_required (VERSION 3.6)

project (MotionDetector)

find_package(Threads)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_COMPILER /usr/bin/clang++)
add_compile_options(-Wall -Werror -pthread)
add_executable(${PROJECT_NAME}  motiondetector.cpp)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})
