set(XAOS_DRIVERS qt gtk x11 win32 cocoa aa)

foreach(DRIVER ${XAOS_DRIVERS})
  add_subdirectory(${DRIVER})
endforeach(DRIVER)
