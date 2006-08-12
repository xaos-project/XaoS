#import "AppController.h"

#include "ui.h"

struct ui_driver osx_driver;

static void
osx_printText(int x, int y, CONST char *text)
{
	[controller printText:text atX:x y:y];
}

static void
osx_refreshDisplay()
{
    [controller refreshDisplay];
}

static void
osx_flipBuffers ()
{
	[controller flipBuffers];
}

void
osx_freeBuffers (char *b1, char *b2)
{
	//[controller freeBuffer1:b1 Buffer2:b2];
}

int
osx_allocBuffers (char **b1, char **b2)
{
	return [controller allocBuffer1:b1 buffer2:b2];
}

static void
osx_getImageSize (int *w, int *h)
{
	[controller getWidth:w height:h];
}

static void
osx_processEvents (int wait, int *mx, int *my, int *mb, int *k)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	
	NSDate *eventDate = wait ? [NSDate distantFuture] : [NSDate distantPast];

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
	//[controller initApp];
    return ( /*1 for sucess 0 for fail */ 1);
}

static void
osx_uninitDriver ()
{
	//[controller unInitApp];
}

static void
osx_getMouse (int *x, int *y, int *b)
{
	[controller getMouseX:x mouseY:y mouseButton:b];
}


static void
osx_setMouseType (int type)
{
	[controller setMouseType:type];
}

void osx_buildMenu (struct uih_context *uih, CONST char *name)
{
	[controller buildMenuWithContext:uih name:name];
}

void osx_toggleMenu (struct uih_context *uih, CONST char *name)
{
	[controller toggleMenuWithContext:uih name:name];
}

void osx_menu (struct uih_context *c, CONST char *name)
{
	//printf("osx_menu\n");
}

void osx_showDialog (struct uih_context *c, CONST char *name)
{
	[controller showDialogWithContext:c name:name];
}

void osx_showHelp (struct uih_context *c, CONST char *name)
{
	[controller showHelpWithContext:c name:name];
}

int main(int argc, char* argv[])
{
	[NSApplication sharedApplication];
	
	[NSBundle loadNibNamed:@"MainMenu" owner:NSApp];
	
	[NSApp finishLaunching];

	XaoS_main(argc, argv);
}

struct gui_driver osx_gui_driver = {
    osx_buildMenu,	// setrootmenu
    osx_toggleMenu,	// enabledisable
    osx_menu,		// menu
    osx_showDialog,	// dialog
    osx_showHelp	// help
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

/* DONT FORGET TO ADD DOCUMENTATION ABOUT YOUR DRIVER INTO xaos.hlp FILE!*/
