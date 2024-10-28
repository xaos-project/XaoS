# This cmake configuration file is based on the work by Werner Volken <werner.volken@vsw.ch>

# To create a native build of XaoS on Linux:
#
# * download a recent Qt SDK (at least Qt 6.2.4 is recommended),
#
# * install CMake (at least CMake 3.14 is suggested),
#
# * type:
#
#   mkdir build; cd build; \
#   CMAKE_PREFIX_PATH=<PATH_TO_QT_SDK>/Qt/6.6.1/gcc_64/lib/cmake/Qt6LinguistTools \
#     cmake -DCMAKE_INSTALL_PREFIX=<INSTALLATION_PATH> .. && \
#   make -j`nprocs`
#
#   where
#   - PATH_TO_QT_SDK is the path of your Qt SDK installation (typically $HOME/Qt),
#   - INSTALLATION_PATH is the planned installation folder of the executable and the supplementary files
#     (say, $HOME/install/xaos, or, if you have root privileges, /usr),
#   - N_PROCS is the number of processors you want to use for the compilation (if you have 8 cores,
#     you may want to use 6).
#
#   You do not have to set CMAKE_PREFIX_PATH if you have the following packages installed
#   (under Ubuntu Linux 22.04):
#
#   - qt6-base-dev
#   - qt6-tools-dev
#   - qt6-tools-dev-tools
#   - qt6-l10n-tools
#   - linguist-qt6
#
#   Also, you do not have to set INSTALLATION_PATH if you do not want to install XaoS system-wide,
#   or you want to use the default setting (/usr/local).
#
# * If the compilation succeeds, you can directly type:
#
#   ./XaoS
#
# * To install XaoS in your INSTALLATION_PATH, type:
#
#   make install
#
#   or, if you want to install it system-wide:
#
#   sudo make install
#
# * To run XaoS from your installation path, type:
#
#   cd <INSTALLATION_PATH>; ./XaoS
#
# * To try the deep zoom feature out (it results in slower animations), add
#
#   -DDEEPZOOM=ON
#
#   on the cmake command line.
#
# * Alternatively, it is possible to build XaoS with the OpenGL driver. For this, you may need
#   additional libraries:
#
#   - freeglut3-dev
#
#   Add
#
#   -DOPENGL=ON
#
#   on the cmake command line to enable the driver.
#
# Notes if you use Qt Creator to build XaoS via cmake:
#
# * You may get an error like 'No CMake configuration for build type "Debug" found.'
#   This is not the real error. Instead, you need to have a look at the Issues window
#   and learn what the problem is. Maybe some package is not found by cmake.
#   In particular, you may need to have libcups2-dev on your system already installed.
#
# * Also, a WebAssembly build is now fully possible, either in Qt Creator,
#   or, via command line:
#
#   mkdir build; cd build; \
#   EMSDK=<PATH_TO_EMSDK> \
#     cmake -DQT_CHAINLOAD_TOOLCHAIN_FILE=<PATH_TO_EMSDK>/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake \
#     -DCMAKE_TOOLCHAIN_FILE=<PATH_TO_QT_SDK>/Qt/6.8.0/wasm_multithread/lib/cmake/Qt6/qt.toolchain.cmake .. &&
#   make -j`nprocs`
#
# If you have an old cmake on your system, use <PATH_TO_QT_SDK>/Tools/CMake/bin/cmake instead of it.

###########################################################################

# because of CMAKE_FIND_PACKAGE_RESOLVE_SYMLINKS
cmake_minimum_required(VERSION 3.14.0)

# WARNING: by setting gcc-13.2 and g++-13.2 it is possible to use quadmath support on Mac:
# set(CMAKE_C_COMPILER gcc-13.2)
# set(CMAKE_CXX_COMPILER g++-13.2)

project(XaoS)

option(DEEPZOOM "Use 128-bit float for deep zoom" OFF)
option(OPENGL "Use OpenGL" OFF)

if(DEEPZOOM)
   add_definitions(-DUSE_FLOAT128) # may be unsupported on Mac, unless gcc/g++ is used
else(DEEPZOOM)
   add_definitions(-DUSE_LONG_DOUBLE)
endif(DEEPZOOM)

if(OPENGL)
   add_definitions(-DUSE_OPENGL)
endif(OPENGL)

# general definitions
add_definitions(-DUSE_SFFE -DSFFE_CMPLX_GSL)

# resolve symbolic links
set(CMAKE_FIND_PACKAGE_RESOLVE_SYMLINKS TRUE)

# by default, build a release (a non-debug executable)
if(NOT CMAKE_BUILD_TYPE)
   set_property(CACHE CMAKE_BUILD_TYPE PROPERTY VALUE Release)
endif(NOT CMAKE_BUILD_TYPE)

# set the project directory
get_filename_component(PROJECT_DIR ${CMAKE_SOURCE_DIR} PATH)

# set-up some QT stuff, they are required for the proper compilation of the GUI
# and automated embedding of the translations in the binary executable
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# look for Qt6
find_package(QT NAMES Qt6 COMPONENTS Widgets PrintSupport REQUIRED)
find_package(Qt6 COMPONENTS Widgets PrintSupport REQUIRED)

if(OPENGL)
   find_package(QT NAMES Qt6 COMPONENTS OpenGL OpenGLWidgets REQUIRED)
   find_package(Qt6 COMPONENTS OpenGL OpenGLWidgets REQUIRED)
   find_package(GLUT REQUIRED)
   find_package(OpenGL REQUIRED)
endif(OPENGL)

# OS specific stuff
# on macOS the QT libraries are usually not installed into the system library folders
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
   set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
   # list(APPEND CMAKE_INSTALL_RPATH ${Qt6_DIR}/../..)
endif()

# set c++ flags
set(CMAKE_CXX_STANDARD 17)

# the include and link paths
include_directories(
   ${CMAKE_CURRENT_SOURCE_DIR}/src/engine
   ${CMAKE_CURRENT_SOURCE_DIR}/src/util
   ${CMAKE_CURRENT_SOURCE_DIR}/src/sffe
   ${CMAKE_CURRENT_SOURCE_DIR}/src/include
   ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
   ${CMAKE_CURRENT_SOURCE_DIR}/src/ui-hlp
)

# set the Application icon, the first line is the property added to Info.plist
set(MACOSX_BUNDLE_ICON_FILE XaoS.icns)
set(XaoS_ICON ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/XaoS.icns)
set_source_files_properties(${XaoS_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

if (EMSCRIPTEN)
else()
# Multilingual support: *.ts -> *.qm
find_package(Qt6LinguistTools)
file(GLOB TRANSLATION_FILES ${CMAKE_CURRENT_SOURCE_DIR}/i18n/*.ts)
# qt_add_translation
set_source_files_properties(${TRANSLATION_FILES}
    PROPERTIES OUTPUT_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/i18n")
qt6_add_translation(QM_FILES ${TRANSLATION_FILES})
endif (EMSCRIPTEN)

# grab all sources for executable
file(GLOB CXX_FILES src/ui/*.cpp src/ui-hlp/*.cpp src/util/*.cpp src/engine/*.cpp src/sffe/*.cpp)
file(GLOB C_FILES src/sffe/*.c)

if(EMSCRIPTEN)
    qt_add_executable(xaos
       ${CXX_FILES}
       ${C_FILES}
       ${QM_FILES}
       ${XaoS_ICON}
       src/ui/XaoS.qrc
       XaoS.qrc
    )
    target_link_libraries(xaos PUBLIC Qt6::Widgets)
    target_link_options(xaos PUBLIC -sASYNCIFY -O3 -flto)
else()
    add_executable(XaoS MACOSX_BUNDLE
       ${CXX_FILES}
       ${C_FILES}
       ${QM_FILES}
       ${XaoS_ICON}
       src/ui/XaoS.qrc
       XaoS.qrc
    )
    target_link_libraries(XaoS Qt6::Widgets Qt6::PrintSupport)
endif(EMSCRIPTEN)

if(DEEPZOOM)
   target_link_libraries(XaoS quadmath)
endif(DEEPZOOM)
if(OPENGL)
   target_link_libraries(XaoS Qt6::OpenGL Qt6::OpenGLWidgets)
endif(OPENGL)

if (EMSCRIPTEN)
    target_link_options(xaos PUBLIC "SHELL:
        --preload-file '${CMAKE_SOURCE_DIR}/catalogs@/catalogs'
        --preload-file '${CMAKE_SOURCE_DIR}/examples@/examples'
        --preload-file '${CMAKE_SOURCE_DIR}/tutorial@/tutorial'")
    add_custom_command(TARGET xaos POST_BUILD
        COMMAND "${CMAKE_SOURCE_DIR}/tools/postprocess-web" "${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}"
        COMMENT "Running postprocess-web..."
    )
else()
# install bundle
install(TARGETS XaoS DESTINATION bin)

# install catalogs and tutorial
install(DIRECTORY catalogs tutorial DESTINATION .)

# install example files
file(GLOB EXAMPLE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/examples/*/*.xpf)
install(FILES ${EXAMPLE_FILES} DESTINATION examples)
endif(EMSCRIPTEN)
