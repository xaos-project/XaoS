#include <stdio.h>
// This C code converts from Microsoft's #13#10 formatted files to UNIX.
// See do-indent for details.
main(int argc, char *argv[])
{
    FILE *f;
    int c;
    f = fopen(argv[1], "r");
    while (!(feof(f))) {
	c = fgetc(f);
	if ((c != 0x0d) && (c != -1))
	    putchar(c);
    }
    fclose(f);
}
