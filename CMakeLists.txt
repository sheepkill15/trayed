cmake_minimum_required(VERSION 3.20)
project(trayed)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

find_library(PSAPI Psapi)

add_executable(trayed src/main.cpp)
#add_subdirectory(helper)

target_link_libraries(trayed Psapi)
#target_link_libraries(trayed helper)
target_include_directories(trayed PRIVATE helper)
#target_include_directories(trayed PRIVATE ${PROJECT_BINARY_DIR}/helper)

add_compile_options(-DPSAPI_VERSION=1)