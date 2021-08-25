cmake_minimum_required(VERSION 3.20)
project(trayed)

set(CMAKE_CXX_STANDARD 17)

find_library(PSAPI Psapi)

add_executable(trayed main.cpp)

target_link_libraries(trayed Psapi)
add_compile_options(-DPSAPI_VERSION=1)