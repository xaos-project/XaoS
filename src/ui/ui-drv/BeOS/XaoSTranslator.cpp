/*	Module for XaoS image translator.	*/

#include <TranslatorAddOn.h>
#include <TranslationKit.h>
#include <ByteOrder.h>
#include <Message.h>
#include <Screen.h>
#include <Locker.h>
#include <FindDirectory.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <CheckBox.h>
#include <Bitmap.h>
#include <TextControl.h>

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fconfig.h>
#include <ctype.h>
#include <setjmp.h>

#include "be_checkfile.h"
#include "be_possitionio.h"
#include "filter.h"
#include "xerror.h"
#include "ui_helper.h"
#include "xthread.h"

//#define CHANGE_ANTIALIAS 'Xaan'

/*#include "colorspace.h"*/
#define INCH 2.54


jmp_buf translatorjmp;
char translatorName[] = "XaoS Translator";
char translatorInfo[] = "Reads the XaoS Position Files and render them to the image. http://www.paru.cas.cz/~hubicka/XaoS/";
int32 translatorVersion = 301; /* format is revision+minor*10+major*100 */
int translator;


#define XPF_TYPE 'xpf '
#define XAF_TYPE 'xaf '


translation_format inputFormats[] = {
	{	XPF_TYPE, B_TRANSLATOR_BITMAP, 0.3, 1.0, "image/x-xaos-position", "XaoS Position File" },
	{	XAF_TYPE, B_TRANSLATOR_BITMAP, 0.01, 1.0, "image/x-xaos-animation", "XaoS Animation File" },
	{	0, 0, 0, 0, "\0", "\0" }
};

translation_format outputFormats[] = {
	{	B_TRANSLATOR_BITMAP, B_TRANSLATOR_BITMAP, 0.4, 0.5, "image/x-be-bitmap", "Be Bitmap Format (XaoSHandler)" },
	{	0, 0, 0, 0, "\0", "\0" }
};

struct XaoS_settings {
	color_space		out_space;
	double			pixelwidth, pixelheight;
	int			width, height;
	bool			antialiasing;
	bool			settings_touched;
};
BLocker g_settings_lock("XaoS settings lock");
extern BLocker g_calculation_lock;
XaoS_settings g_settings;


class PrefsLoader {
public:
		PrefsLoader(
				const char * str)
			{
				g_settings_lock.Lock();
				/* defaults */
				g_settings.out_space = B_NO_COLOR_SPACE;
				g_settings.width = XSIZE;
				g_settings.height = YSIZE;
				g_settings.antialiasing = 0;
				g_settings.pixelwidth =  0.02979;
				g_settings.pixelheight =  0.02979;
				g_settings.settings_touched = false;
				BPath path;
				if (find_directory(B_USER_SETTINGS_DIRECTORY, &path)) {
					path.SetTo("/tmp");
				}
				path.Append(str);
				FILE * f = fopen(path.Path(), "r");
				if (f) {
					char line[1024];
					char name[32];
					char * ptr;
					while (true) {
						line[0] = 0;
						fgets(line, 1024, f);
						if (feof(f)) break;

						ptr = line;
						while (isspace(*ptr)) {
							ptr++;
						}
						if (!*ptr) continue;
						if (*ptr == '#' || !*ptr) {	/* comment or blank */
							continue;
						}
						if (sscanf(ptr, "%31[a-zA-Z_0-9] =", name) != 1) {
							fprintf(stderr, "unknown XaoSTranslator settings line: %s", line);
						}
						else {
							if (!strcmp(name, "color_space")) {
								while (*ptr != '=') {
									ptr++;
								}
								ptr++;
								if (sscanf(ptr, "%d", (int*)&g_settings.out_space) != 1) {
									fprintf(stderr, "illegal color space in XaoSTranslator settings: %s", ptr);
								}
							}
							else if (!strcmp(name, "antialiasing")) {
								while (*ptr != '=') {
									ptr++;
								}
								ptr++;
								int antialiasing = g_settings.antialiasing;
								if (sscanf(ptr, "%d", &antialiasing) != 1) {
									fprintf(stderr, "illegal antialiasing value in XaoSTranslator settings: %s", ptr);
								}
								else {
									g_settings.antialiasing = antialiasing;
								}
							}
							else if (!strcmp(name, "size")) {
								while (*ptr != '=') {
									ptr++;
								}
								ptr++;
								int width = g_settings.width;
								int height = g_settings.height;
								if (sscanf(ptr, "%d,%d", &width, &height) != 2 || width<1 || width>4096 || height<1 || height>4096) {
									fprintf(stderr, "illegal size value in XaoSTranslator settings: %s", ptr);
								}
								else {
									g_settings.width = width;
									g_settings.height = height;
								}
							}
							else if (!strcmp(name, "pixelsize")) {
								while (*ptr != '=') {
									ptr++;
								}
								ptr++;
								double pixelwidth = g_settings.pixelwidth;
								double pixelheight = g_settings.pixelheight;
								if (sscanf(ptr, "%lg,%lg", &pixelwidth, &pixelheight) != 2 || pixelwidth<=0 || pixelheight<=0) {
									fprintf(stderr, "illegal size value in XaoSTranslator settings: %s", ptr);
								}
								else {
									g_settings.pixelwidth = pixelwidth;
									g_settings.pixelheight = pixelheight;
								}
							}
							else {
								fprintf(stderr, "unknown XaoSTranslator setting: %s", line);
							}
						}
					}
					fclose(f);
				}
				g_settings_lock.Unlock();
			}
		~PrefsLoader()
			{
				/*	No need writing settings if there aren't any	*/
				if (g_settings.settings_touched) {
					BPath path;
					if (find_directory(B_USER_SETTINGS_DIRECTORY, &path)) {
						path.SetTo("/tmp");
					}
					path.Append("XaoSTranslator_Settings");
					FILE * f = fopen(path.Path(), "w");
					if (f) {
						fprintf(f, "# XaoSTranslator settings version %.2f\n", (float)translatorVersion/100.0);
						fprintf(f, "color_space = %d\n", g_settings.out_space);
						fprintf(f, "antialiasing = %d\n", g_settings.antialiasing ? 1 : 0);
						fprintf(f, "size = %d,%d\n", g_settings.width, g_settings.height);
						fprintf(f, "pixelsize = %g,%g\n", g_settings.pixelwidth, g_settings.pixelheight);
						fclose(f);
					}
				}
			}
};

PrefsLoader g_prefs_loader("XaoSTranslator_Settings");
static color_space optimaloutspace(color_space out_space)
{
	switch (out_space) {	
                /*The 24-bit versions are not compiled in, because BeOS don't
		  support them in most cases. Use 32 instead.	*/
		case B_RGB24:
		case B_RGB24_BIG:
			out_space = B_RGB32;
		case B_RGB32: break;
		case B_RGB32_BIG: break;
		case B_RGBA32: out_space = B_RGB32; break;
		case B_RGBA32_BIG: out_space = B_RGB32_BIG; break;
		case B_RGB16: 
		case B_RGB16_BIG: 
			if (B_HOST_IS_LENDIAN) out_space = B_RGB16; 
				else out_space = B_RGB16_BIG;
			break;
		case B_RGBA15: 
		case B_RGBA15_BIG:
		case B_RGB15: 
		case B_RGB15_BIG: 
			if (B_HOST_IS_LENDIAN) out_space = B_RGB15; 
				else out_space = B_RGB15_BIG;
			break;
		case B_CMAP8: out_space = B_RGB32; break; 
			/*Support for dithering is fast but don't looks
			  perfectly. It is better to keep it up to client app */
		case B_GRAY8: break; 
		case B_GRAY1: break; 
	
		break;
		default:
			/* use best supported colorspace and let client app to convert it*/
			out_space=B_RGB32;
			break;
	}
	return out_space;
}

extern void be_get_imagespecs(int cs, int *imagetype, int *rmask, int *gmask, int *bmask);


/* This function is expected to identify input source and say whether it is
   correct or not. XaoS files don't have any particular headers so this test
   is done by looking for opening bracket (so we refuse empty files) and if
   that success interpreting first frame. In case first few commands are XaoS
   commands, it is very high probability that the rest is correct too.
   So mistakes are extremly inprobable I believe
   
   Hope this behaviour is what user want. 

*/
status_t
Identify(	/*	required	*/
	BPositionIO * inSource,
	const translation_format * inFormat,	/*	can beNULL	*/
	BMessage * ioExtension,	/*	can be NULL	*/
	translator_info * outInfo,
	uint32 outType)
{
	/* Silence compiler warnings. */
	inFormat = inFormat;
	ioExtension = ioExtension;
	int r;

	/* Check that requested format is something we can deal with. */
	if (outType == 0) {
		outType = B_TRANSLATOR_BITMAP;
	}
	if (outType != B_TRANSLATOR_BITMAP) {
		return B_NO_TRANSLATOR;
	}

	outInfo->group = B_TRANSLATOR_BITMAP;
	outInfo->type = B_TRANSLATOR_BITMAP;
	outInfo->quality = 0.4;
	outInfo->capability = 0.8;
	strcpy(outInfo->name, "Be Bitmap Format (XaoSHandler)");
	strcpy(outInfo->MIME, "image/x-be-bitmap");
	

	g_settings_lock.Lock();
        if(setjmp(translatorjmp)) {
                /* OOPS something went realy badly. Try to return to app.
                   hopefully it will work. */
                xth_uninit();
                g_calculation_lock.Unlock();
                return 0;
        }
	r=XaoSCheckFile(inSource);
	g_settings_lock.Unlock();
	return (r?B_OK:B_NO_TRANSLATOR);
}



status_t
Translate(	
	BPositionIO * inSource,
	const translator_info * inInfo,
	BMessage * ioExtension,
	uint32 outType,
	BPositionIO * outDestination)
{
	status_t err=B_OK;
	bool headeronly=false, dataonly=false;

	inSource->Seek(0, SEEK_SET);
	inInfo = inInfo;

	if (!outType) {
		outType = B_TRANSLATOR_BITMAP;
	}
	if (outType != B_TRANSLATOR_BITMAP && outType != XPF_TYPE && outType != XAF_TYPE) {
		return B_NO_TRANSLATOR;
	}

	color_space out_space, real_out_space;
	int out_rowbytes;
	g_settings_lock.Lock();
	int width,height;
	bool anti;
	double pixelwidth, pixelheight;
	if (!ioExtension || ioExtension->FindInt32("xpf /width", (int32*)&width)) width=g_settings.width;
	if (!ioExtension || ioExtension->FindInt32("xpf /height", (int32*)&height)) height=g_settings.height;
	if (!ioExtension || ioExtension->FindDouble("xpf /pixelwidth", &pixelwidth)) pixelwidth=g_settings.pixelwidth;
	if (!ioExtension || ioExtension->FindDouble("xpf /pixelheight", &pixelheight)) pixelheight=g_settings.pixelheight;
	if (!ioExtension || ioExtension->FindBool("xpf /antialiasing", &anti)) anti=g_settings.antialiasing;
	if (!ioExtension || ioExtension->FindInt32(B_TRANSLATOR_EXT_BITMAP_COLOR_SPACE, (int32*)&out_space))
	 {	
		out_space = g_settings.out_space;
		if (out_space == B_NO_COLOR_SPACE) {
			out_space = B_RGB32;
		}
	 }
	real_out_space = out_space;
	if (!ioExtension || ioExtension->FindBool(B_TRANSLATOR_EXT_HEADER_ONLY, &headeronly)) headeronly=false;

	if (!ioExtension || ioExtension->FindBool(B_TRANSLATOR_EXT_DATA_ONLY, &dataonly)) dataonly=false;

	out_space = optimaloutspace(out_space);
	
        /* Find the most similar colorspace supported by XaoS engine */
	g_settings_lock.Unlock();

        /* Construct image settings */
	int imagetype;
	union paletteinfo info;
        be_get_imagespecs(out_space, (int *)&imagetype, (int *)&info.truec.rmask, (int *)&info.truec.gmask, (int *)&info.truec.bmask);
	out_rowbytes=bytesperpixel(imagetype)*width;
	if (!out_rowbytes) out_rowbytes=(width+7)/8;

	/* B_TRANSLATOR_BITMAP header */
	TranslatorBitmap hdr;
	hdr.magic = B_HOST_TO_BENDIAN_INT32(B_TRANSLATOR_BITMAP);
	hdr.bounds.left = B_HOST_TO_BENDIAN_FLOAT(0);
	hdr.bounds.top = B_HOST_TO_BENDIAN_FLOAT(0);
	hdr.bounds.right = B_HOST_TO_BENDIAN_FLOAT(width-1);
	hdr.bounds.bottom = B_HOST_TO_BENDIAN_FLOAT(height-1);
	hdr.rowBytes = B_HOST_TO_BENDIAN_INT32(out_rowbytes);
	hdr.colors = (color_space)B_HOST_TO_BENDIAN_INT32(out_space);
	hdr.dataSize = B_HOST_TO_BENDIAN_INT32(out_rowbytes*height);
	if (!dataonly) err = outDestination->Write(&hdr, sizeof(hdr));
	
	if (headeronly) return B_OK;
	struct palette *pal=createpalette(0,0, imagetype, 0, 0, NULL, NULL, NULL, NULL, &info);
	char *bits = new char[out_rowbytes*height];
	if (!pal)
	{
		x_error("Can not create palette (out of memory)\n");
		return B_NO_TRANSLATOR;
	}
	struct image *image=create_image_cont(width, height, out_rowbytes,2, (pixel_t *)bits, (pixel_t *)bits, pal, flipgeneric, 0, pixelwidth, pixelheight);
	if (!image)
	{
		x_error("Can not create image (out of memory)");
		destroypalette(pal);
		return B_NO_TRANSLATOR;
	}
	xio_file input=positionio_ropen(inSource);
	g_calculation_lock.Lock();
	translator=1;
	if(setjmp(translatorjmp)) {
		/* OOPS something went realy badly. 
                   Try to return to application. Hopefully it will work.*/
		xth_uninit();
        	g_calculation_lock.Unlock();
		return B_NO_TRANSLATOR;
	}
	xio_init("");
        uih_registermenus();
	uih_renderimage(NULL, input, NULL, image, anti, NULL, 2 /*output messages */);
	destroypalette(pal);
	xio_uninit();
	xth_uninit();
	uih_unregistermenus();
	translator=0;
	g_calculation_lock.Unlock();
	outDestination->Write(bits, (image->currlines[1]-image->currlines[0])*height);
	destroy_image(image);
	delete bits;
	return B_OK;
}

class XaoSTranslatorView :
	public BView
{
public:
		int visible;
		XaoSTranslatorView(
				const BRect & frame,
				const char * name,
				uint32 resize,
				uint32 flags,	
				BMessage *ioExtension) :
			BView(frame, name, resize, flags)
			{
				visible=0;
				/*if (ioExtension) SetSettings(ioExtension);*/
				g_settings_lock.Lock();
				SetViewColor(220,220,220,0);
				mMenu = new BPopUpMenu("Color Space");
				mMenu->AddItem(new BMenuItem("None", CSMessage(B_NO_COLOR_SPACE)));
				mMenu->AddItem(new BMenuItem("RGB 8:8:8 32 bits", CSMessage(B_RGB32)));
				if (B_HOST_IS_LENDIAN) {
					mMenu->AddItem(new BMenuItem("RGB 5:5:5 16 bits", CSMessage(B_RGB15)));
					mMenu->AddItem(new BMenuItem("RGB 5:6:5 16 bits", CSMessage(B_RGB16)));
				}
				mMenu->AddSeparatorItem();
				mMenu->AddItem(new BMenuItem("Grayscale 8 bits", CSMessage(B_GRAY8)));
				mMenu->AddItem(new BMenuItem("Bitmap 1 bit", CSMessage(B_GRAY1)));
				mMenu->AddSeparatorItem();
				mMenu->AddItem(new BMenuItem("RGB 8:8:8 32 bits big-endian", CSMessage(B_RGB32_BIG)));
				if (B_HOST_IS_BENDIAN) {
					mMenu->AddItem(new BMenuItem("RGB 5:5:5 16 bits big-endian", CSMessage(B_RGB15_BIG)));
					mMenu->AddItem(new BMenuItem("RGB 5:6:5 16 bits big-endian", CSMessage(B_RGB16_BIG)));
				}
				mField = new BMenuField(BRect(20,70,180,90), "Color Space Field", "Color Space", mMenu);
				mField->SetViewColor(ViewColor());
				AddChild(mField);
				SelectColorSpace(g_settings.out_space);
				BMessage * msg = new BMessage(CHANGE_ANTIALIAS);
				mAntialiasing = new BCheckBox(BRect(20,45,180,62), "Antialiasing", "Antialiasing", msg);
				if (g_settings.antialiasing) {
					mAntialiasing->SetValue(1);
				}
				mAntialiasing->SetViewColor(ViewColor());
				AddChild(mAntialiasing);
				char s[256];
				sprintf(s,"%i",g_settings.width);
				mWidth = new BTextControl(BRect(20, 70+35, 180, 70+35+17), "Width", "Width", s, new BMessage(CHANGE_WIDTH));
				mWidth->SetViewColor(ViewColor());
				AddChild(mWidth);
				sprintf(s,"%i",g_settings.height);
				mHeight = new BTextControl(BRect(20, 60+2*35, 180, 60+2*35+17), "Height", "Height", s, new BMessage(CHANGE_HEIGHT));
				mHeight->SetViewColor(ViewColor());
				AddChild(mHeight);
				sprintf(s,"%i",(int)(INCH/g_settings.pixelwidth+0.5));
				mPixelWidth = new BTextControl(BRect(20, 70+3*35, 180, 70+3*35+17), "X DPI", "X DPI", s, new BMessage(CHANGE_PIXELWIDTH));
				mPixelWidth->SetViewColor(ViewColor());
				AddChild(mPixelWidth);
				sprintf(s,"%i",(int)(INCH/g_settings.pixelheight+0.5));
				mPixelHeight = new BTextControl(BRect(20, 60+4*35, 180, 60+4*35+17), "Y DPI", "Y DPI", s, new BMessage(CHANGE_PIXELHEIGHT));
				mPixelHeight->SetViewColor(ViewColor());
				AddChild(mPixelHeight);
				visible=1;
				SelectColorSpace(g_settings.out_space);
				g_settings_lock.Unlock();
			}
		~XaoSTranslatorView()
			{
				/* nothing here */
			}

		enum {
			SET_COLOR_SPACE = 'xpf=',
			CHANGE_ANTIALIAS,
			CHANGE_WIDTH,
			CHANGE_HEIGHT,
			CHANGE_PIXELWIDTH,
			CHANGE_PIXELHEIGHT
		};

virtual	void Draw(
				BRect area)
			{
				area = area; /* silence compiler */
				SetFont(be_bold_font);
				font_height fh;
				GetFontHeight(&fh);
				char str[100];
				sprintf(str, "XaoSTranslator %.2f", (float)translatorVersion/100.0);
				DrawString(str, BPoint(fh.descent+1, fh.ascent+fh.descent*2+fh.leading));
			}
virtual	void MessageReceived(
				BMessage * message)
			{
				if (message->what == SET_COLOR_SPACE) {
					SetSettings(message);
				}
				else if (message->what == CHANGE_ANTIALIAS) {
					BMessage msg;
					msg.AddBool("xpf /antialiasing", mAntialiasing->Value());
					SetSettings(&msg);
				}
				else if (message->what == CHANGE_WIDTH) {
					BMessage msg;
					int i;
					if(sscanf(mWidth->Text(),"%i",&i)==1 && i>0 && i<4096)
					 msg.AddInt32("xpf /width", i), SetSettings(&msg);
				}
				else if (message->what == CHANGE_HEIGHT) {
					BMessage msg;
					int i;
					if(sscanf(mHeight->Text(),"%i",&i)==1 && i>0 && i<4096)
					 msg.AddInt32("xpf /height", i), SetSettings(&msg);
				}
				else if (message->what == CHANGE_PIXELWIDTH) {
					BMessage msg;
					int i;
					if(sscanf(mPixelWidth->Text(),"%i",&i)==1 && i>0) {
					 msg.AddDouble("xpf /pixelwidth", INCH/(double)i), SetSettings(&msg);
					}
				}
				else if (message->what == CHANGE_PIXELHEIGHT) {
					BMessage msg;
					int i;
					if(sscanf(mPixelHeight->Text(),"%i",&i)==1 && i>0)
					 msg.AddDouble("xpf /pixelheight",INCH/(double)i), SetSettings(&msg);
				}
				else {
					BView::MessageReceived(message);
				}
			}
virtual	void AllAttached()
			{
				BView::AllAttached();
				BMessenger msgr(this);
				/*	Tell all menu items we're the man.	*/
				for (int ix=0; ix<mMenu->CountItems(); ix++) {
					BMenuItem * i = mMenu->ItemAt(ix);
					if (i) {
						i->SetTarget(msgr);
					}
				}
				mAntialiasing->SetTarget(msgr);
				mWidth->SetTarget(msgr);
				mHeight->SetTarget(msgr);
				mPixelWidth->SetTarget(msgr);
				mPixelHeight->SetTarget(msgr);
			}

		void SetSettings(
				BMessage * message)
			{
				g_settings_lock.Lock();
				color_space space;
				if (!message->FindInt32(B_TRANSLATOR_EXT_BITMAP_COLOR_SPACE, (int32*)&space)) {
					g_settings.out_space = space;
					SelectColorSpace(space);
					g_settings.settings_touched = true;
				}
				bool antialiasing;
				if (!message->FindBool("xpf /antialiasing", &antialiasing)) {
					g_settings.antialiasing = antialiasing;
					g_settings.settings_touched = true;
				}
				int32 width;
				if (!message->FindInt32("xpf /width", &width)) {
					g_settings.width = width;
					g_settings.settings_touched = true;
				}
				int32 height;
				if (!message->FindInt32("xpf /height", &height)) {
					g_settings.height = height;
					g_settings.settings_touched = true;
				}
				double pixelwidth;
				if (!message->FindDouble("xpf /pixelwidth", &pixelwidth)) {
					g_settings.pixelwidth = pixelwidth;
					g_settings.settings_touched = true;
				}
				double pixelheight;
				if (!message->FindDouble("xpf /pixelheight", &pixelheight)) {
					g_settings.pixelheight = pixelheight;
					g_settings.settings_touched = true;
				}
				g_settings_lock.Unlock();
			}

private:
		BPopUpMenu * mMenu;
		BMenuField * mField;
		BCheckBox * mAntialiasing;
		BTextControl * mWidth;
		BTextControl * mHeight;
		BTextControl * mPixelWidth;
		BTextControl * mPixelHeight;

		BMessage * CSMessage(
				color_space space)
			{
				BMessage * ret = new BMessage(SET_COLOR_SPACE);
				ret->AddInt32(B_TRANSLATOR_EXT_BITMAP_COLOR_SPACE, space);
				return ret;
			}

		void SelectColorSpace(
				color_space space)
			{
				if (!visible) return;
				for (int ix=0; ix<mMenu->CountItems(); ix++) {
					int32 s;
					BMenuItem * i = mMenu->ItemAt(ix);
					if (i) {
						BMessage * m = i->Message();
						if (m && !m->FindInt32(B_TRANSLATOR_EXT_BITMAP_COLOR_SPACE, &s) && (s == space)) {
							mMenu->Superitem()->SetLabel(i->Label());
							break;
						}
					}
				}
			}
};

status_t 
MakeConfig(	/*	optional	*/
	BMessage * ioExtension,	/*	can be NULL	*/
	BView * * outView,
	BRect * outExtent)
{
	XaoSTranslatorView * v = new XaoSTranslatorView(BRect(0,0,200,100), "XaoSTranslator Settings", B_FOLLOW_ALL, B_WILL_DRAW, ioExtension);
	*outView = v;
	*outExtent = v->Bounds();
	return B_OK;
}

status_t
GetConfigMessage(	/*	optional	*/
	BMessage * ioExtension)
{
	status_t err = B_OK;
	const char * name = B_TRANSLATOR_EXT_BITMAP_COLOR_SPACE;
	g_settings_lock.Lock();
	(void)ioExtension->RemoveName(name);
	err = ioExtension->AddInt32(name, g_settings.out_space);
	err = ioExtension->AddInt32("xpf /width", g_settings.width);
	err = ioExtension->AddInt32("xpf /height", g_settings.height);
	err = ioExtension->AddDouble("xpf /pixelwidth", g_settings.pixelwidth);
	err = ioExtension->AddDouble("xpf /pixelheight", g_settings.pixelheight);
	err = ioExtension->AddBool("xpf /antialiasing", g_settings.antialiasing);
	g_settings_lock.Unlock();
	return err;
}





