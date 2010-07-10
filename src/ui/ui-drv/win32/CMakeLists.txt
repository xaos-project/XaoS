if (WIN32)
  option(USE_WIN32 "Build Win32 User Interface" ON)
endif (WIN32)

if(${USE_WIN32})
  set (UIWIN32_SRC
     ui_win32.c 
     dxguid.c 
     winmenu.c
     windialo.c
     about.c
     xaos.rc)

  add_executable(xaos WIN32 ${UIWIN32_SRC})
endif(WIN32)
