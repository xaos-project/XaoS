#		requires djgpp(2.0) and allegro(2.1)

#for egcs 1.1.x
CFLAGS1= -D__OPTIMIZE__ -Wall -mpentium -fstrength-reduce -ffast-math -fomit-frame-pointer -fno-exceptions -O6 -funroll-loops -frerun-loop-opt -fstrict-aliasing  -L. -D DOG_DRIVER  -Wall -D AA_DRIVER -malign-double
#for gcc
#CFLAGS1= -mpentium -O3 -fstrength-reduce -ffast-math -funroll-loops -fforce-mem -fforce-addr -fomit-frame-pointer -L.  -D DOG_DRIVER  -Wall -D AA_DRIVER
#for pgcc
#for most of files:
#CFLAGS1= -fopt-reg-stack -mpentium -O6 -fdo-offload -frecombine -frisc -freg-reg-copy-opt -fswap-for-agi  -floop-after-global -fschedule-stack-reg-insns -fopt-reg-use -fpeep-spills -fstrength-reduce -ffast-math -funroll-loops -fforce-mem -fforce-addr -fomit-frame-pointer -fno-exceptions -fmove-all-movables -L.  -D DOG_DRIVER  -Wall
#for fractal.c:
#CFLAGS1= -mpentium -O6 -fdo-offload -frecombine -frisc -freg-reg-copy-opt -fswap-for-agi  -floop-after-global -fschedule-stack-reg-insns -fopt-reg-use -fpeep-spills -fstrength-reduce -ffast-math -funroll-loops -fforce-mem -fforce-addr -fomit-frame-pointer -fno-exceptions -fmove-all-movables -L.  -D DOG_DRIVER  -Wall
LIBS = -lalleg -L ../../vga -L ../../text -L ../../aalib-1.2 -laa -ltext -lvga -lpng -lz -lm
LFLAGS = -s

