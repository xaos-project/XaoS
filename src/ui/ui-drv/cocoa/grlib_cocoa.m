#include <config.h>

#import <Cocoa/Cocoa.h>

#include <fconfig.h>
#include <filter.h>
#include <fractal.h>
#include <ui_helper.h>
#include <grlib.h>

static NSMutableDictionary *textAttributes(int fgcolor, int bgcolor) {
    
    float red, green, blue;
    
    NSMutableDictionary *attrsDictionary = [NSMutableDictionary 
					    dictionaryWithCapacity:5];
    
    [attrsDictionary setValue:[NSColor whiteColor] 
		       forKey:NSForegroundColorAttributeName];
    
    [attrsDictionary setValue:[NSFont 
			       boldSystemFontOfSize:[NSFont systemFontSize]] 
		       forKey:NSFontAttributeName];
    
    //NSLog(@"%x", fgcolor);
    red   = (fgcolor & RMASK) / 255.0;
    green = (fgcolor & GMASK) / 255.0;
    blue  = (fgcolor & BMASK) / 255.0;
    [attrsDictionary setValue:[NSColor colorWithDeviceRed:red 
						    green:green 
						     blue:blue 
						    alpha:1.0] 
		       forKey:NSForegroundColorAttributeName];
    
    red   = (bgcolor & RMASK) / 255.0;
    green = (bgcolor & GMASK) / 255.0;
    blue  = (bgcolor & BMASK) / 255.0;
    NSShadow *textShadow = [[NSShadow alloc] init];
    [textShadow setShadowColor:[NSColor colorWithDeviceRed:red 
						     green:green 
						      blue:blue 
						     alpha:1.0]];
    [textShadow setShadowOffset:NSMakeSize(2, -2)];
    [textShadow setShadowBlurRadius:1];
    
    [attrsDictionary setValue:textShadow forKey:NSShadowAttributeName];
    [textShadow autorelease];
    
    return attrsDictionary;
}


static int
xprint(struct image *image, CONST struct xfont *current, int x, int y,
       CONST char *text, int fgcolor, int bgcolor, int mode)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    
    NSBitmapImageRep *imageRep = [[NSBitmapImageRep alloc] 
				  initWithBitmapDataPlanes:image->currlines
				  pixelsWide:image->width
				  pixelsHigh:image->height
				  bitsPerSample:8
				  samplesPerPixel:3
				  hasAlpha:NO
				  isPlanar:NO
				  colorSpaceName:NSDeviceRGBColorSpace
				  bytesPerRow:0
				  bitsPerPixel:32];
    
    [NSGraphicsContext saveGraphicsState];
    NSGraphicsContext *context = [NSGraphicsContext 
				  graphicsContextWithBitmapImageRep:imageRep];
    [NSGraphicsContext setCurrentContext:context];
    
    NSString *messageText = [[[NSString stringWithUTF8String:text] 
			      componentsSeparatedByString:@"\n"] 
			     objectAtIndex:0];
    //NSLog(messageText);
    
    NSMutableDictionary *attrsDictionary = textAttributes(fgcolor, bgcolor);
    
    NSSize textSize = [messageText sizeWithAttributes:attrsDictionary];
    [messageText drawAtPoint:NSMakePoint(x, image->height - y - textSize.height) 
	      withAttributes:attrsDictionary];
    
    [NSGraphicsContext restoreGraphicsState];
    
    /* 
     * The calling functions expect the return value to be the number of
     * bytes in the string, not the number of characters (this is not always
     * equal for UTF-8 strings).  Therefore, we don't use [messageText length].
     */
    int bytesUsed = [messageText 
			 lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
    
    [imageRep release];
    [pool release];
    
    return bytesUsed;
}

static int xtextwidth(CONST struct xfont *font, CONST char *text)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    
    NSString *messageText = [[[NSString stringWithUTF8String:text] 
			      componentsSeparatedByString:@"\n"] 
			     objectAtIndex:0];
    
    NSMutableDictionary *attrsDictionary = textAttributes(0, 0);
    
    NSSize textSize = [messageText sizeWithAttributes:attrsDictionary];
    
    [pool release];
    return ceil(textSize.width) + 2;
}

static int xtextheight(CONST struct xfont *font) {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    
    NSString *messageText = @"Test String";
    
    NSMutableDictionary *attrsDictionary = textAttributes(0, 0);
    NSSize textSize = [messageText sizeWithAttributes:attrsDictionary];
    [pool release];
    return ceil(textSize.height) + 2;
}

static int xtextcharw(CONST struct xfont *font, CONST char c)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    
    NSString *messageText = [NSString stringWithFormat:@"%c", c];
    
    NSMutableDictionary *attrsDictionary = textAttributes(0, 0);
    NSSize textSize = [messageText sizeWithAttributes:attrsDictionary];
    [pool release];
    return ceil(textSize.width) + 2;
}

void register_grlib_cocoa(void) {
	grlib.xprint = xprint;
	grlib.xtextwidth = xtextwidth;
	grlib.xtextheight = xtextheight;
	grlib.xtextcharw = xtextcharw;
}

