#		requires djgpp(2.0) and allegro(2.1)

#for egcs 1.1.x
CFLAGS1= -D__OPTIMIZE__ -Wall -mpentium -fstrength-reduce -ffast-math -fomit-frame-pointer -fno-exceptions -Os -fstrict-aliasing -L.  -D DOG_DRIVER  -Wall -D AA_DRIVER -malign-double
#for PGCC
#CFLAGS1= -mpentium -O2 -fstrength-reduce -ffast-math -fforce-mem -fforce-addr -fomit-frame-pointer -fno-exceptions -fmove-all-movables -I ../../../../../aalib-1.2  -L.  -D DOG_DRIVER  -Wall
LIBS = -lalleg -L ../../vga -L ../../text -L ../../aalib-1.2 -laa -ltext -lvga -lpng -lz -lm
LFLAGS = -s

