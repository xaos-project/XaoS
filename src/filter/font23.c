#include "xfont16.c"

// Doubles the xfont16 font for a bigger output of font (xfont48)
// By Zoltan Kovacs <kovzol@particio.com>, 2006-04-25

// Compile it with "gcc -o font23 font23.c" and then run it with
// "./font23 > xfont48.c; indent xfont48.c".

main()
{
    int a, b, i, j, k;
    printf("#include <config.h>\nCONST unsigned char xfont48[] = {\n");

    for (i = 0; i < 256; ++i)	// 256 characters
    {
	for (j = 0; j < 16; ++j)	// 16 lines vertically
	{
	    a = xfont16[i * 16 + j];	// 8 bits of graphics read
	    b = 0;		// this will be the output
	    for (k = 0; k < 8; ++k) {
		b /= 4;
		if (a % 2 == 1)	// if the most right bit is set
		    b += 49152;	// the the output will be also set, twice
		a /= 2;
	    }
	    printf("0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x",
		   (b / 256), (b % 256), (b / 256),
		   (b % 256), (b / 256), (b % 256));
	    if (!(i == 255 && j == 15))
		printf(", ");
	}			// end of character
	printf("\n");
    }
    printf("};\n");

}
