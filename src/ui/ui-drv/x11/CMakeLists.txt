find_package(X11)

if(X11_FOUND)
  option(USE_X11 "Build X11 GUI" ON)
endif(X11_FOUND)
