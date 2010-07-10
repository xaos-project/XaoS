set (FILTER_SRC
     image.c
     palette.c
     random.c
     grlib.c
     font.c
     filter.c)

add_library (filter STATIC ${FILTER_SRC})
