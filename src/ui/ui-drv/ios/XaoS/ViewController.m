//
//  ViewController.m
//  XaoS
//
//  Created by   on 5/22/13.
//
//



#import "ViewController.h"



struct ui_driver ios_driver;

static NSData* pxBuffers[2];
static bool currentBuffer;

static CIContext* ciContext;
static GLKView* glkView;


static CGRect renderRect;

static CGPoint mousePos;
static int mouseButton;

static bool needsResize;

static ViewController* vc;
static UIScrollView* scrollView;

static UILabel* statusLabel;

static CGFloat textCaretY;

static bool hadCursor, hasCursor;

extern void showkb(float y){
    hadCursor = hasCursor = 1;
    textCaretY = y / glkView.contentScaleFactor;
    dispatch_async(dispatch_get_main_queue(), ^{
        [vc becomeFirstResponder];
    });
};

extern void hidekb(){
    dispatch_async(dispatch_get_main_queue(), ^{
        [vc resignFirstResponder];
    });
}


@implementation AppViewController

- (void) loadView {
    scrollView = [[UIScrollView alloc] initWithFrame:(UIInterfaceOrientationIsLandscape([UIApplication sharedApplication].statusBarOrientation)
                                                    ? CGRectMake(0,0, [UIScreen mainScreen].bounds.size.height, [UIScreen mainScreen].bounds.size.width)
                                                     : CGRectMake(0,0, [UIScreen mainScreen].bounds.size.width, [UIScreen mainScreen].bounds.size.height))];
    scrollView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    scrollView.translatesAutoresizingMaskIntoConstraints = scrollView.autoresizesSubviews = YES;
    scrollView.contentSize = scrollView.bounds.size;
    
    
   
    
    [self addChildViewController:(vc = [ViewController new])];
    [scrollView addSubview:vc.view];
    [vc didMoveToParentViewController:self];
    
    statusLabel = [[UILabel alloc] initWithFrame:CGRectMake(0, scrollView.bounds.size.height - 20, scrollView.bounds.size.width, 20)];
    statusLabel.backgroundColor = [UIColor colorWithRed:0 green:0 blue:0 alpha:0.5];
    statusLabel.textColor = [UIColor whiteColor];
    statusLabel.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleTopMargin;
    statusLabel.textAlignment = UITextAlignmentCenter;
    statusLabel.font = [UIFont boldSystemFontOfSize:12];
    
    [scrollView addSubview:statusLabel];
    [scrollView bringSubviewToFront:statusLabel];
    
    self.view = scrollView;
}

- (void) willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration {
    scrollView.contentSize = scrollView.bounds.size;
}
@end


@implementation ViewController

- (void)loadView {
    self.preferredFramesPerSecond = 60;
    
    self.view = glkView = [GLKView new];
    glkView.DrawableDepthFormat = GLKViewDrawableDepthFormat24;
    
    glkView.frame = (UIInterfaceOrientationIsLandscape([UIApplication sharedApplication].statusBarOrientation)
                     ? CGRectMake(0,0, [UIScreen mainScreen].bounds.size.height, [UIScreen mainScreen].bounds.size.width)
                     : CGRectMake(0,0, [UIScreen mainScreen].bounds.size.width, [UIScreen mainScreen].bounds.size.height));
    
    glkView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    glkView.translatesAutoresizingMaskIntoConstraints = glkView.autoresizesSubviews = YES;
    glkView.multipleTouchEnabled = glkView.userInteractionEnabled = YES;
    
    ios_driver.width = ios_driver.height = glkView.contentScaleFactor * ((UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad) ? (1/335.28) : (1/414.02));
    
    UIPinchGestureRecognizer* pinch = [[UIPinchGestureRecognizer alloc] initWithTarget:self action:@selector(userDidPinch:)];
    pinch.delegate = self;
    [glkView addGestureRecognizer:pinch];
    
    
    UIPanGestureRecognizer* pan = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(userDidPan:)];
    pan.minimumNumberOfTouches = pan.maximumNumberOfTouches = 1;
    pan.delegate = self;
    [glkView addGestureRecognizer:pan];
    
    
    UITapGestureRecognizer* tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(userDidTap:)];
    tap.delegate = self;
    [glkView addGestureRecognizer:tap];
    
    
    
    
    
    
    
    renderRect = CGRectMake(0,0,
                         glkView.bounds.size.width * glkView.contentScaleFactor,
                         glkView.bounds.size.height * glkView.contentScaleFactor);
    


    
    
    
    
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(keyboardWillShow:)
                                                 name:UIKeyboardWillShowNotification
                                               object:nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(keyboardWillHide:)
                                                 name:UIKeyboardWillHideNotification
                                               object:nil];
    
    [[NSNotificationCenter defaultCenter]addObserver:self
                                            selector:@selector(applicationDidBecomeActive)
                                                name:UIApplicationDidBecomeActiveNotification
                                              object:nil];
    
    [EAGLContext setCurrentContext:
     (glkView.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2])
     ];
    
    glDisable(GL_COLOR_LOGIC_OP);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_DITHER);
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_MULTISAMPLE);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    glDisable(GL_SAMPLE_ALPHA_TO_ONE);
    glDisable(GL_SAMPLE_COVERAGE);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);
    
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_FOG);
    glDisable(GL_TEXTURE_2D);
    
    
    ciContext = [CIContext contextWithEAGLContext:glkView.context options:@{kCIContextWorkingColorSpace: [NSNull null]}];
    
    [self runXaosMain];
}

- (UIKeyboardAppearance) keyboardAppearance {
    return UIKeyboardAppearanceAlert;
}


- (void) keyboardWillShow: (NSNotification*)aNotification{
    NSTimeInterval duration;
    [aNotification.userInfo[UIKeyboardAnimationDurationUserInfoKey] getValue:&duration];
    
    
    CGFloat kbHeight = [scrollView convertRect:[aNotification.userInfo[UIKeyboardFrameBeginUserInfoKey] CGRectValue] toView:nil].size.height;
    scrollView.scrollIndicatorInsets = scrollView.contentInset =
    UIEdgeInsetsMake(0.0, 0.0,
                     kbHeight,
                     0.0);
    
    [UIView animateWithDuration:duration animations:^{
        scrollView.contentOffset = CGPointMake(0.0, MIN(MAX(0,textCaretY - (scrollView.bounds.size.height - kbHeight) / 2),
                                                        kbHeight));
    }];
}

- (void) keyboardWillHide:(NSNotification*)aNotification{
    
    NSTimeInterval duration;
    [aNotification.userInfo[UIKeyboardAnimationDurationUserInfoKey] getValue:&duration];
    
    scrollView.scrollIndicatorInsets = scrollView.contentInset = UIEdgeInsetsZero;
}
     
    

- (void)insertText:(NSString *)text {
    ui_key([text characterAtIndex:0]);
}
- (void)deleteBackward {
    ui_key(UIKEY_BACKSPACE);
}

- (BOOL)hasText {
    return YES;
}

- (BOOL) canBecomeFirstResponder {
    return YES;
}

- (void) applicationDidBecomeActive {
    ciContext = [CIContext contextWithEAGLContext:glkView.context options:@{kCIContextWorkingColorSpace: [NSNull null]}];
}

- (void) willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration {
    ciContext = [CIContext contextWithEAGLContext:glkView.context options:@{kCIContextWorkingColorSpace: [NSNull null]}];
    needsResize = 1;
}

- (void) runXaosMain {
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
        chdir(@"~/Documents".stringByExpandingTildeInPath.UTF8String);
        MAIN_FUNCTION(gargc, gargv);
    });
}

- (BOOL) gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer {
    return YES;
}

- (void) userDidPinch:(UIPinchGestureRecognizer*) sender {
    if (sender.state == UIGestureRecognizerStateCancelled
        || sender.state == UIGestureRecognizerStateFailed
        || sender.state == UIGestureRecognizerStatePossible
        || sender.state == UIGestureRecognizerStateEnded
        || sender.numberOfTouches != 2) return;
    
    
    
    
    CGPoint loc = [sender locationInView:glkView];
    mousePos = CGPointMake(loc.x * glkView.contentScaleFactor, loc.y * glkView.contentScaleFactor);
    mouseButton =
        sender.velocity
        ? (sender.velocity > 0 ? BUTTON1 : BUTTON3)
        : 0;
    globaluih->maxstep = ABS(sender.velocity);
}

- (void) userDidPan:(UIPanGestureRecognizer*) sender {
    if (sender.state == UIGestureRecognizerStateCancelled
        || sender.state == UIGestureRecognizerStateFailed
        || sender.state == UIGestureRecognizerStatePossible) return;
    
    if (sender.state == UIGestureRecognizerStateEnded){
        mouseButton = 0;
        return;
    }
    
    CGPoint loc = [sender locationOfTouch:0 inView:glkView];
    mousePos = CGPointMake(loc.x * glkView.contentScaleFactor, loc.y * glkView.contentScaleFactor);
    
    //if (!(mouseButton & (BUTTON1 | BUTTON3)))
    mouseButton = BUTTON2;
}

- (void) userDidTap:(UITapGestureRecognizer*) sender {
    if (sender.state == UIGestureRecognizerStateCancelled || sender.state == UIGestureRecognizerStateFailed || sender.state == UIGestureRecognizerStatePossible) return;
    
    CGPoint loc = [sender locationInView:glkView];
    mousePos = CGPointMake(loc.x * glkView.contentScaleFactor, loc.y * glkView.contentScaleFactor);
    
    mouseButton = BUTTON1;
}



- (void) glkView:(GLKView *)view drawInRect:(CGRect)rect {
    if (!ciContext) return;
    
    glClear(GL_COLOR_BUFFER_BIT);
    
    [ciContext drawImage:
        [CIImage imageWithBitmapData:pxBuffers[currentBuffer] bytesPerRow:(renderRect.size.width * 4) size:renderRect.size format:kCIFormatARGB8 colorSpace:nil]
                  inRect:renderRect fromRect:renderRect];
}

@end












static void
ios_printText(int x, int y, CONST char *text)
{
    NSString* str = @(text).uppercaseString;
    dispatch_async(dispatch_get_main_queue(),^{
        if (str.length){
            if (statusLabel.alpha == 0){
                statusLabel.hidden = NO;
                [UIView animateWithDuration:0.16 animations:^{
                    statusLabel.alpha = 1;
                }];
            }
            statusLabel.text = str;
        }else if (statusLabel.alpha)
            [UIView animateWithDuration:0.16 animations:^{
                statusLabel.alpha = 0;
            } completion:^(BOOL finished) {
                statusLabel.hidden = YES;
            }];
    });
}

static void
ios_refreshDisplay()
{
    if (hadCursor){
        if (hasCursor){
            hasCursor = 0;   
        }else{
            hidekb();
            hadCursor = 0;
        }
    }
}

static void
ios_flipBuffers ()
{
    currentBuffer ^= 1;
}

static void
ios_freeBuffers (char *b1, char *b2)
{
    pxBuffers[0] = pxBuffers[1] = nil;
}

static int
ios_allocBuffers (char **b1, char **b2)
{
    currentBuffer = 0;
    
    int rowLen = renderRect.size.width * 4;
    int imgLen = rowLen * renderRect.size.height;
    
    pxBuffers[0] = [NSData dataWithBytesNoCopy:(*b1 = malloc(imgLen)) length:imgLen];
    pxBuffers[1] = [NSData dataWithBytesNoCopy:(*b2 = malloc(imgLen)) length:imgLen];
    
    return rowLen;
}




static void
ios_getImageSize (int *w, int *h)
{
    *w = renderRect.size.width;
    *h = renderRect.size.height;
}




static int
ios_initDriver ()
{
    return 1;
}

static int
ios_initFullScreenDriver ()
{
    return 1;
}

static void
ios_uninitDriver ()
{
    
}

static void
ios_getMouse (int *x, int *y, int *b)
{
    *x = mousePos.x;
    *y = mousePos.y;
    if ((*b = mouseButton) & (~BUTTON2)){
        mouseButton = 0;
    }
}

static void
ios_processEvents (int wait, int *mx, int *my, int *mb, int *k)
{
    if (needsResize){
        renderRect = CGRectMake(0,0,
                                glkView.bounds.size.width * glkView.contentScaleFactor,
                                glkView.bounds.size.height * glkView.contentScaleFactor);
        ui_resize();
        needsResize = 0;
    }
    *mx = mousePos.x;
    *my = mousePos.y;
    *k = 0;
    if ((*mb = mouseButton) & (~BUTTON2)){
        mouseButton = 0;
    }
}


//static void
//ios_setCursorType (int type)
//{
//    
//}

//static void
//ios_buildMenu (struct uih_context *uih, CONST char *name)
//{
//    
//}
//
//static void
//ios_showPopUpMenu (struct uih_context *c, CONST char *name)
//{
//    
//}
//
//
//static void
//ios_showDialog (struct uih_context *c, CONST char *name)
//{
//    
//}
//
//static void
//ios_showHelp (struct uih_context *c, CONST char *name)
//{
//    
//}
//
//
//struct gui_driver ios_gui_driver = {
//    /* setrootmenu */   ios_buildMenu,
//    /* enabledisable */ NULL,
//    /* menu */          ios_showPopUpMenu,
//    /* dialog */        ios_showDialog,
//    /* help */          ios_showHelp
//};


static struct params ios_params[] = {
	{NULL, 0, NULL, NULL}
};

struct ui_driver ios_driver = {
    /* name */          "iOS Driver",
    /* init */          ios_initDriver,
    /* getsize */       ios_getImageSize,
    /* processevents */ ios_processEvents,
    /* getmouse */      ios_getMouse,
    /* uninit */        ios_uninitDriver,
    /* set_color */     NULL,
    /* set_range */     NULL,
    /* print */         ios_printText,
    /* display */       ios_refreshDisplay,
    /* alloc_buffers */ ios_allocBuffers,
    /* free_buffers */  ios_freeBuffers,
    /* filp_buffers */  ios_flipBuffers,
    /* mousetype */     NULL,
    /* flush */         NULL,
    /* textwidth */     12,
    /* textheight */    12,
    /* params */        ios_params,
    /* flags */         PIXELSIZE,
    /* width */         0.0,
    /* height */        0.0,
    /* maxwidth */      0,
    /* maxheight */     0,
    /* imagetype */     UI_TRUECOLOR,
    /* palettestart */  0,
    /* paletteend */    256,
    /* maxentries */    255,
#if __BIG_ENDIAN__
    /* rmask */         0xff000000,
    /* gmask */         0x00ff0000,
    /* bmask */         0x0000ff00,
#else
    /* rmask */         0x000000ff,
    /* gmask */         0x0000ff00,
    /* bmask */         0x00ff0000,
#endif
    /* gui_driver */    NULL//&ios_gui_driver
};
/* DONT FORGET TO ADD DOCUMENTATION ABOUT YOUR DRIVER INTO xaos.hlp FILE!*/