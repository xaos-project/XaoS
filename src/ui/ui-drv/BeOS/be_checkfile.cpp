#include <ctype.h>
#include <setjmp.h>
#include "be_checkfile.h"
#include "be_possitionio.h"
#include "filter.h"
#include "ui_helper.h"
#include "ui.h"
#include "xthread.h"
#include "xerror.h"
extern int translator;
extern jmp_buf translatorjmp;

/* Our goal is to check specified file and say whether it is XaoS file or not */
BLocker g_calculation_lock("XaoS calculation lock");
int XaoSCheckFile(BPositionIO * inSource)
{
	char c=' ';
	int r;

	if(!inSource->Read(&c,1)) return 0;
	while(c!='(') /* XaoS commands starts by left quote - ugly test, but can avoid simple mistakes */
		      /* this also makes us to refuse empty file, that is in fact valid XaoS input file.
			 hope this is not problem. */
	{
		/* skip comments */
		if (c==';') {
		   while(c!='\n')
	              if(!inSource->Read(&c,1)) return 0;
		}
		/* tab, space, enter and line feed are alowed between commands */
		if (!isspace(c) && c!='\r' && c!='\n') return 0;
	        if(!inSource->Read(&c,1)) return 0;


		/* OK W've passed first test. Now try to interpret it if thats works, accept it*/
	}

	inSource->Seek(0, SEEK_SET);
	translator=1;
        if(setjmp(translatorjmp)) {
                /* OOPS something went realy badly. Try to return to app.
                   hopefully it will work. */
                xth_uninit();
                g_calculation_lock.Unlock();
                return 0;
        }
	union paletteinfo info;
	//color_space out_space = optimaloutspace(g_settings.out_space);
	info.truec.rmask=0xff;
	info.truec.gmask=0xff00;
	info.truec.bmask=0xff0000;
	struct palette *pal=createpalette(0,0, TRUECOLOR, 0, 0, NULL, NULL, NULL, NULL, &info);
	if (!pal)
	{
		x_error("Can not create palette (out of memory)\n");
		return 0;
	}
	struct image *image=create_image_mem(1, 1, 2,  pal, (float)1, (float)1);
	if (!image)
	{
		x_error("Can not create image (out of memory)");
		destroypalette(pal);
		return 0;
	}
        g_calculation_lock.Lock();
	xio_file input=positionio_ropen(inSource);
       	uih_registermenus(), 
	xth_init(0);
	xio_init("");
	r=uih_renderimage(NULL, input, NULL, image, 0, NULL, 0 /*output messages */);
	destroy_image(image);
	destroypalette(pal);
	xio_uninit();
	xth_uninit();
	uih_unregistermenus();
	translator=0;
        g_calculation_lock.Unlock();
	return (r);
}

