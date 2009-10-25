#include "fractalfacade.h"
#include <config.h>
#include <filter.h>
#include <ui_helper.h>

int uih_message(uih_context * c, CONST char *message)
{
    FractalFacade *f = reinterpret_cast<FractalFacade*>(c->data);
    f->showMessage(message);
    return 0;
}

int uih_error(uih_context * c, CONST char *error)
{
    FractalWidget *f = reinterpret_cast<FractalFacade*>(c->data);
    f->showError(error);
    return 0;
}

void uih_clearmessages(uih_context * c)
{
    FractalFacade *f = reinterpret_cast<FractalFacade*>(c->data);
    f->clearMessage();
}

void uih_initmessages(uih_context * c) {}
void uih_destroymessages(uih_context * c) {}
void uih_printmessages(uih_context * c) {}

FractalFacade::FractalFacade(UIAdapter &a):
        ui(a)
{
    uih = uih_mkcontext(0, createImage(), NULL, NULL, NULL);
    uih->data = this;
    //uih->fcontext->version++;
    //uih_loadcatalog(uih, "english");
    uih_newimage(uih);
}

FractalFacade::~FractalFacade()
{
    destroypalette(uih->image->palette);
    destroy_image(uih->image);
    uih_freecontext(uih);
}

struct image *FractalWidget::createImage()
{

    union paletteinfo info;
    info.truec.rmask = ui.rmask();
    info.truec.gmask = ui.gmask();
    info.truec.bmask = ui.bmask();

    struct palette *pal = createpalette (0, 0, TRUECOLOR, 0, 0, NULL,
                                         NULL, NULL, NULL, &info);

    struct image *img =
    create_image_cont(image[0].width(), image[0].height(),
                      image[0].bytesPerLine(), 2,
                      image[0].bits(),
                      image[1].bits(),
                      pal, NULL,
                      0,
                      0.01,
                      0.01);

    return img;
}
