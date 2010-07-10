include_directories(include)

#add_subdirectory(i18n)

set (XAOS_LIBRARIES ui ui-hlp engine util filter) # sffe

foreach(LIB ${XAOS_LIBRARIES})
  add_subdirectory(${LIB})
endforeach(LIB)
