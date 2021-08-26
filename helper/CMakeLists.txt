project(helper)

set(SOURCE_LIB helper.cpp)
include_directories (include ${PROJECT_BINARY_DIR})

include(GenerateExportHeader)

add_library(helper SHARED ${SOURCE_LIB})

set_target_properties(helper PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

GENERATE_EXPORT_HEADER(helper
        BASE_NAME helper
        EXPORT_MACRO_NAME helper_EXPORT
        EXPORT_FILE_NAME helper_Export.h
        STATIC_DEFINE helper_BUILT_AS_STATIC)