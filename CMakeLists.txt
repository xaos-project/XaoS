# This cmake configuration file is based on the work by Werner Volken <werner.volken@vsw.ch>

# To create a native build of XaoS on Linux:
#
# * download a recent Qt SDK (at least Qt 6.2.4 is recommended),
#
# * install CMake (at least CMake 3.14 is suggested),
#
# * type:
#
#   mkdir build; cd build \
#   CMAKE_PREFIX_PATH=<PATH_TO_QT_SDK>/Qt/6.6.1/gcc_64/lib/cmake/Qt6LinguistTools \
#     cmake -DCMAKE_INSTALL_PREFIX=<INSTALLATION_PATH> .. && \
#   make -j<N_PROCS>
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

###########################################################################

# because of CMAKE_FIND_PACKAGE_RESOLVE_SYMLINKS
cmake_minimum_required(VERSION 3.14.0)

# WARNING: by setting gcc-13.2 and g++-13.2 it is possible to use quadmath support on Mac:
# set(CMAKE_C_COMPILER gcc-13.2)
# set(CMAKE_CXX_COMPILER g++-13.2)

project(XaoS)

option(DEEPZOOM "Use 128-bit float for deep zoom" OFF)
if(DEEPZOOM)
   add_definitions(-DUSE_FLOAT128) # may be unsupported on Mac, unless gcc/g++ is used
else(DEEPZOOM)
   add_definitions(-DUSE_LONG_DOUBLE)
endif(DEEPZOOM)

add_definitions(-DUSE_SFFE -DSFFE_CMPLX_GSL)

# resolve symbolic links
set(CMAKE_FIND_PACKAGE_RESOLVE_SYMLINKS TRUE)

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

# Multilingual support: *.ts -> *.qm
find_package(Qt6LinguistTools)
file(GLOB TRANSLATION_FILES ${CMAKE_CURRENT_SOURCE_DIR}/i18n/*.ts)

# qt_add_translation
set_source_files_properties(${TRANSLATION_FILES}
    PROPERTIES OUTPUT_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/i18n")
qt6_add_translation(QM_FILES ${TRANSLATION_FILES})

# grab all sources for executable
file(GLOB CXX_FILES src/ui/*.cpp src/ui-hlp/*.cpp src/util/*.cpp src/engine/*.cpp src/sffe/*.cpp)
file(GLOB C_FILES src/sffe/*.c)
add_executable(XaoS MACOSX_BUNDLE
   ${CXX_FILES}
   ${C_FILES}
   ${QM_FILES}
   ${XaoS_ICON}
   src/ui/XaoS.qrc
   XaoS.qrc
)

# Link
target_link_libraries(XaoS Qt6::Widgets Qt6::PrintSupport)
if(DEEPZOOM)
target_link_libraries(XaoS quadmath)
endif(DEEPZOOM)

# install bundle
install(TARGETS XaoS DESTINATION bin)

# install translations
install(FILES DESTINATION bin/XaoS.app/Contents/Resources/translations)

# install catalogs and tutorial
install(DIRECTORY catalogs tutorial DESTINATION bin/XaoS.app/Contents/Resources)

# install example files
file(GLOB EXAMPLE_FILES ${PROJECT_DIR}/examples/*/*.xpf)
install(FILES ${EXAMPLE_FILES} DESTINATION bin/XaoS.app/Contents/Resources/examples)
