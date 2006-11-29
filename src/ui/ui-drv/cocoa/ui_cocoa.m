#import "AppController.h"

#include "ui.h"

struct ui_driver osx_driver;

static void
osx_printText(int x, int y, CONST char *text)
{
    printf("osx_printText\n");
	[controller printText:text atX:x y:y];
}

static void
osx_refreshDisplay()
{
    printf("osx_refreshDisplay\n");
    [controller refreshDisplay];
}

static void
osx_flipBuffers ()
{
    printf("osx_flipBuffers\n");
	[controller flipBuffers];
}

void
osx_freeBuffers (char *b1, char *b2)
{
    printf("osx_freeBuffers\n");
	[controller freeBuffers];
}

int
osx_allocBuffers (char **b1, char **b2)
{
    printf("osx_allocBuffers\n");
	return [controller allocBuffer1:b1 buffer2:b2];
}

static void
osx_getImageSize (int *w, int *h)
{
    printf("osx_getImageSize\n");
	[controller getWidth:w height:h];
}

static void
osx_processEvents (int wait, int *mx, int *my, int *mb, int *k)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	
	//NSDate *eventDate = wait ? [NSDate distantFuture] : [NSDate distantPast];
	
	NSEvent *event = [NSApp nextEventMatchingMask: NSAnyEventMask
										untilDate: nil
										   inMode: NSDefaultRunLoopMode
										  dequeue: YES];
	
	if (event != nil)
	{
	    [NSApp sendEvent: event];
	}
	
	[pool release];
	
	[controller getMouseX:mx mouseY:my mouseButton:mb keys:k];
}


static int
osx_initDriver ()
{	
    printf("osx_initDriver\n");
	osx_init(0);
    return ( /*1 for sucess 0 for fail */ 1);
}

static int
osx_initDriverFull ()
{	
    printf("osx_initDriverFull\n");
	osx_init(1);
    return ( /*1 for sucess 0 for fail */ 1);
}

static void
osx_uninitDriver ()
{
    printf("osx_uninitDriver\n");
	//[controller unInitApp];
}

static void
osx_getMouse (int *x, int *y, int *b)
{
    printf("osx_getMouse\n");
	[controller getMouseX:x mouseY:y mouseButton:b];
}


static void
osx_setMouseType (int type)
{
    printf("osx_setMouseType\n");
	[controller setMouseType:type];
}

void osx_buildMenu (struct uih_context *uih, CONST char *name)
{
    printf("osx_buildMenu\n");
	[controller buildMenuWithContext:uih name:name];
}

void osx_toggleMenu (struct uih_context *uih, CONST char *name)
{
    printf("osx_toggleMenu\n");
	[controller toggleMenuWithContext:uih name:name];
}

void osx_menu (struct uih_context *c, CONST char *name)
{
    printf("osx_menu\n");
	//printf("osx_menu\n");
}


void osx_showDialog (struct uih_context *c, CONST char *name)
{
    printf("osx_showDialog\n");
	[controller showDialogWithContext:c name:name];
}

void osx_showHelp (struct uih_context *c, CONST char *name)
{
    printf("osx_showHelp\n");
	[controller showHelpWithContext:c name:name];
}

int main(int argc, char* argv[])
{
	[NSApplication sharedApplication];
	
	[NSBundle loadNibNamed:@"MainMenu" owner:NSApp];
	
	[NSApp finishLaunching];
	
	return XaoS_main(argc, argv);
}

struct gui_driver osx_gui_driver = {
    osx_buildMenu,	// setrootmenu
    osx_toggleMenu,	// enabledisable
    NULL,		// menu
    osx_showDialog,	// dialog
    NULL	// help
};


static struct params osx_params[] = {
	{NULL, 0, NULL, NULL}
};

struct ui_driver osx_driver = {
    /* name */          "Mac OS X Driver",
    /* init */          osx_initDriver,
    /* getsize */       osx_getImageSize,
    /* processevents */ osx_processEvents,
    /* getmouse */      osx_getMouse,
    /* uninit */        osx_uninitDriver,
    /* set_color */     NULL,
    /* set_range */     NULL,
    /* print */         osx_printText,
    /* display */       osx_refreshDisplay,
    /* alloc_buffers */ osx_allocBuffers,
    /* free_buffers */  osx_freeBuffers,
    /* filp_buffers */  osx_flipBuffers,
    /* mousetype */     osx_setMouseType,
    /* flush */         NULL,
    /* textwidth */     12,
    /* textheight */    12,
    /* params */        osx_params,
    /* flags */         RESOLUTION | PIXELSIZE,
    /* width */         0.01, 
    /* height */        0.01,
    /* maxwidth */      0, 
    /* maxheight */     0,
    /* imagetype */     UI_TRUECOLOR,
    /* palettestart */  0, 
    /* paletteend */    256, 
    /* maxentries */    255,
    /* rmask */         0xff000000,
    /* gmask */         0x00ff0000,
    /* bmask */         0x0000ff00,
    /* gui_driver */    &osx_gui_driver
};

struct ui_driver osx_fullscreen_driver = {
    /* name */          "Mac OS X Fullscreen Driver",
    /* init */          osx_initDriverFull,
    /* getsize */       osx_getImageSize,
    /* processevents */ osx_processEvents,
    /* getmouse */      osx_getMouse,
    /* uninit */        osx_uninitDriver,
    /* set_color */     NULL,
    /* set_range */     NULL,
    /* print */         osx_printText,
    /* display */       osx_refreshDisplay,
    /* alloc_buffers */ osx_allocBuffers,
    /* free_buffers */  osx_freeBuffers,
    /* filp_buffers */  osx_flipBuffers,
    /* mousetype */     osx_setMouseType,
    /* flush */         NULL,
    /* textwidth */     12,
    /* textheight */    12,
    /* params */        osx_params,
    /* flags */         RESOLUTION | PIXELSIZE,
    /* width */         0.01, 
    /* height */        0.01,
    /* maxwidth */      0, 
    /* maxheight */     0,
    /* imagetype */     UI_TRUECOLOR,
    /* palettestart */  0, 
    /* paletteend */    256, 
    /* maxentries */    255,
    /* rmask */         0xff000000,
    /* gmask */         0x00ff0000,
    /* bmask */         0x0000ff00,
    /* gui_driver */    &osx_gui_driver
};

int osx_init (int fullscreen)
{
    struct ui_driver    *driver;
    
    driver = fullscreen ? &osx_fullscreen_driver : &osx_driver;
	
    // The following is necessary to ensure correct colors on Intel-based Macs
    // Determine if machine is little-endian and if so swap color mask bytes
	{
		unsigned char c[4];
		*(unsigned short *) c = 0xff;
		if (c[0] == (unsigned char) 0xff) {
			int shift = 0;
#define SWAPE(c)  (((c&0xffU)<<24)|((c&0xff00U)<<8)|((c&0xff0000U)>>8)|((c&0xff000000U)>>24))
			driver->rmask = SWAPE (driver->rmask) >> shift;
			driver->gmask = SWAPE (driver->gmask) >> shift;
			driver->bmask = SWAPE (driver->bmask) >> shift;
		}
	}
	return 0;
    
}

/* DONT FORGET TO ADD DOCUMENTATION ABOUT YOUR DRIVER INTO xaos.hlp FILE!*/
