find_package(GTK2 2.8)

if(GTK2_FOUND)
  option(USE_GTK "Build GTK User Interface" ON)
endif(GTK2_FOUND)
