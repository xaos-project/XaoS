#include <config.h>
#ifndef _plan9_
#include <aconfig.h>
#ifdef USE_PNG
#include <zlib.h>
#include <png.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifndef NO_MALLOC_H
#include <malloc.h>
#endif
#include <errno.h>
#else
#include <u.h>
#include <libc.h>
#include <stdio.h>
#endif
#include <filter.h>
#include <version.h>
#include <misc-f.h>
#include <xio.h>
#ifndef USE_PNG
const char *writepng(xio_constpath filename, const struct image *img)
{
    if (img->driver && img->driver->saveimage)
        return img->driver->saveimage(img, filename);

    return
	"XaoS can not save images because it was incorrectly compiled. Please compile it with zlib and libpng";
}
#else

const char *writepng(xio_constpath filename, const struct image *image)
{
    png_structp png_ptr;
    png_infop info_ptr;
    png_color palette[256];
    png_color_8 sig_bit;
    volatile unsigned short a = 255;
    volatile unsigned char *b = (volatile unsigned char *) &a;
#ifdef _undefined_
    static char text[] =
	"XaoS" XaoS_VERSION " - a realtime interactive fractal zoomer";
    static png_text comments[] = {
	{
	 -1,
	 "Software",
	 text,
	 sizeof(text)}
    };
#endif
    FILE *file = fopen(filename, "wb");
    errno = -1;
    if (file == NULL) {
	return strerror(errno);
    }
    png_ptr =
	png_create_write_struct(PNG_LIBPNG_VER_STRING, (void *) NULL,
				(png_error_ptr) NULL,
				(png_error_ptr) NULL);
    if (!png_ptr)
	return "Unable to initialize pnglib";
    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
	png_destroy_write_struct(&png_ptr, (png_infopp) NULL);
	return "No memory to create png info structure";
    }
    if (setjmp(png_jmpbuf(png_ptr))) {
	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(file);
	return strerror(errno);
    }
    png_init_io(png_ptr, file);

#ifdef _undefined_
    png_set_filter(png_ptr, 0,
		   PNG_FILTER_NONE | PNG_FILTER_SUB | PNG_FILTER_PAETH |
		   PNG_FILTER_UP | PNG_FILTER_AVG);
    /* set the zlib compression level */
    /*png_set_compression_level(png_ptr, Z_BEST_COMPRESSION); */
    png_set_compression_level(png_ptr, Z_DEFAULT_COMPRESSION);

    /* set other zlib parameters */
    png_set_compression_mem_level(png_ptr, 8);
    png_set_compression_strategy(png_ptr, Z_DEFAULT_STRATEGY);
    png_set_compression_window_bits(png_ptr, 15);
    png_set_compression_method(png_ptr, 8);
#endif

    switch (image->palette->type) {
    case C256:
	{
	    int i;
            png_set_IHDR(png_ptr, info_ptr, image->width, image->height, 
                    image->bytesperpixel * 8, PNG_COLOR_TYPE_PALETTE, 
                    PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, 
                    PNG_FILTER_TYPE_DEFAULT);
	    for (i = 0; i < image->palette->end; i++) {
		palette[i].red = image->palette->rgb[i][0];
	        palette[i].green = image->palette->rgb[i][1];
		palette[i].blue = image->palette->rgb[i][2];
            }
            png_set_PLTE(png_ptr, info_ptr, palette, image->palette->end);
	}
	break;
    case SMALLITER:
    case LARGEITER:
    case GRAYSCALE:
        png_set_IHDR(png_ptr, info_ptr, image->width, image->height, 
                image->bytesperpixel * 8, PNG_COLOR_TYPE_GRAY, 
                PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, 
                PNG_FILTER_TYPE_DEFAULT);
	break;
    case TRUECOLOR:
    case TRUECOLOR24:
    case TRUECOLOR16:
        png_set_IHDR(png_ptr, info_ptr, image->width, image->height, 
                8, PNG_COLOR_TYPE_RGB, 
                PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, 
                PNG_FILTER_TYPE_DEFAULT);
	sig_bit.red = 8 - image->palette->info.truec.rprec;
	sig_bit.green = 8 - image->palette->info.truec.gprec;
	sig_bit.blue = 8 - image->palette->info.truec.bprec;
        png_set_sBIT(png_ptr, info_ptr, &sig_bit);
	break;
    }
#ifdef _undefined_
    png_set_text(png_ptr, info_ptr, comments,
		 sizeof(comments) / sizeof(png_text));
#endif
    png_write_info(png_ptr, info_ptr);
    /*png_set_filler(png_ptr,0,PNG_FILLER_AFTER); */
    png_set_packing(png_ptr);
    if (image->palette->type & (TRUECOLOR | TRUECOLOR24 | TRUECOLOR16))
	png_set_shift(png_ptr, &sig_bit);
    if (*b == 255)
	png_set_swap(png_ptr);
    png_set_bgr(png_ptr);
    switch (image->palette->type) {
    case C256:
    case GRAYSCALE:
    case SMALLITER:
    case LARGEITER:
#ifdef STRUECOLOR24
    case TRUECOLOR24:
	png_write_image(png_ptr, (png_bytepp) image->currlines);
	break;
#endif
    case TRUECOLOR:
	{
	    int i, y;
	    unsigned char *r = (unsigned char *) malloc(image->width * 3);
	    for (i = 0; i < image->height; i++) {
		for (y = 0; y < image->width; y++)
		    r[y * 3 + 2] =
			(((pixel32_t **) (image->
					  currlines))[i][y] &
			 image->palette->info.truec.rmask) >> image->
			palette->info.truec.rshift, r[y * 3 + 1] =
			(((pixel32_t **) (image->
					  currlines))[i][y] &
			 image->palette->info.truec.gmask) >> image->
			palette->info.truec.gshift, r[y * 3] =
			(((pixel32_t **) (image->
					  currlines))[i][y] &
			 image->palette->info.truec.bmask) >> image->
			palette->info.truec.bshift;
		png_write_rows(png_ptr, (png_bytepp) & r, 1);
	    }
	}
	break;
#ifdef SUPPORT16
    case TRUECOLOR16:
	{
	    int i, y;
	    unsigned char *r = (unsigned char *) malloc(image->width * 3);
	    for (i = 0; i < image->height; i++) {
		for (y = 0; y < image->width; y++)
		    r[y * 3 + 2] =
			(((pixel16_t **) (image->
					  currlines))[i][y] &
			 image->palette->info.truec.rmask) >> image->
			palette->info.truec.rshift, r[y * 3 + 1] =
			(((pixel16_t **) (image->
					  currlines))[i][y] &
			 image->palette->info.truec.gmask) >> image->
			palette->info.truec.gshift, r[y * 3] =
			(((pixel16_t **) (image->
					  currlines))[i][y] &
			 image->palette->info.truec.bmask) >> image->
			palette->info.truec.bshift;
		png_write_rows(png_ptr, (png_bytepp) & r, 1);
	    }
	}
	break;
#endif
    }
    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(file);
    return NULL;
}
#endif
