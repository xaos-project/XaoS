cmake_minimum_required (VERSION 2.6)
project (XaoS)

set (XaoS_VERSION_MAJOR 3)
set (XaoS_VERSION_MINOR 6)

#configure_file ("${PROJECT_SOURCE_DIR}/src/include/aconfig.h.in"
#		"${PROJECT_BINARY_DIR}/src/include/aconfig.h")

#include_directories("${PROJECT_BINARY_DIR}/src/include")

add_subdirectory (src)
