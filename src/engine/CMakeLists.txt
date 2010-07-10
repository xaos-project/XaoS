set (ENGINE_SRC
     formulas.c
     fractal.c
     btrace.c
     palettef.c
     emboss.c
     star.c
     anti.c
     dither.c
     edge.c
     edge2.c
     rotate.c
     zoom.c
     blur.c
     interlace.c
     itersmall.c
     stereogram.c
     3d.c
     subwindow.c
     plane.c
     julia.c
     i386.c)

add_library(engine STATIC ${ENGINE_SRC})
