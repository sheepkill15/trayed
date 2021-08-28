cmake_minimum_required(VERSION 3.20)
project(trayed)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

find_library(PSAPI Psapi)

add_executable(trayed src/main.cpp src/win_tray.cpp src/win_tray.h)
#add_subdirectory(helper)

target_link_libraries(trayed Psapi)
target_link_libraries(trayed -static)
#target_link_libraries(trayed helper)
#target_include_directories(trayed PRIVATE helper)

add_compile_options(-DPSAPI_VERSION=1)