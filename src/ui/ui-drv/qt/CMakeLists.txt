find_package(Qt4 4.4.3 COMPONENTS QtCore QtGui)
include(${QT_USE_FILE})

if(QT4_FOUND)
  option(USE_QT "Build Qt GUI" ON)
endif(QT4_FOUND)

if(USE_QT) 
  set (UIQT_SRC
     main.cpp
     mainwindow.cpp
     fractalwidget.cpp
     customdialog.cpp)

  set (UIQT_MOC_HEADERS
     mainwindow.h
     fractalwidget.h
     customdialog.h)

  qt4_wrap_cpp (UIQT_MOC_OUTFILES ${UIQT_MOC_HEADERS})

  add_executable(xaos ${UIQT_SRC} ${UIQT_MOC_OUTFILES})
  add_dependencies(xaos ${XAOS_LIBRARIES})
  target_link_libraries(xaos ${XAOS_LIBRARIES})
  target_link_libraries(xaos ${QT_LIBRARIES})
endif(USE_QT)
