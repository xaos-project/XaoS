set (UTIL_SRC
     png.c
     catalog.c
     thread.c
     xstring.c
     help.c
     xerror.c
     xshl.c
     xldio.c
     xstdio.c
     xmenu.c
     timers.c)

add_library (util STATIC ${UTIL_SRC})
