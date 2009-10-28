#include "abstractfractalmodel.h"

#include <signal.h>

#include "config.h"
#include "xio.h"
#include "xthread.h"
#include "xerror.h"
#include "filter.h"
#include "formulas.h"
#include "plane.h"
#include "ui_helper.h"

int uih_message(uih_context * c, CONST char *message)
{
    AbstractFractalModel *m = reinterpret_cast<AbstractFractalModel*>(c->data);
    m->showMessage(message);
    return 0;
}

int uih_error(uih_context * c, CONST char *error)
{
    AbstractFractalModel *m = reinterpret_cast<AbstractFractalModel*>(c->data);
    m->showError(error);
    return 0;
}

void uih_clearmessages(uih_context * c)
{
    AbstractFractalModel *m = reinterpret_cast<AbstractFractalModel*>(c->data);
    m->clearMessage();
}

void uih_initmessages(uih_context * c) {}
void uih_destroymessages(uih_context * c) {}
void uih_printmessages(uih_context * c) {}

int ui_passfunc(struct uih_context *c, int display, CONST char *text,
            float percent)
{
    AbstractFractalModel *m = reinterpret_cast<AbstractFractalModel*>(c->data);
    m->passFunc(display, text, percent);
}

void AbstractFractalModel::initApp(int argc, char **argv)
{
    xio_init(argv[0]);
    uih_registermenudialogs_i18n();
    uih_registermenus_i18n();
    uih_registermenus();
    signal(SIGFPE, SIG_IGN);
    //xth_init(1);
    //srand(time(NULL));
}

AbstractFractalModel::AbstractFractalModel()
{

    m_redMask = 0;
    m_greenMask = 0;
    m_blueMask = 0;
    m_imageWidth = 0;
    m_imageHeight = 0;
    m_bytesPerLine = 0;
    m_buffer1 = 0;
    m_buffer2 = 0;
    m_pixelWidth = 0;
    m_pixelHeight = 0;

    m_mouseX = 0;
    m_mouseY = 0;
    m_mouseButtons = 0;

    allocateBuffers();

    m_uih = uih_mkcontext(0, createImage(), ui_passfunc, NULL, NULL);
    m_uih->data = this;
    //m_uih->fcontext->version++;
    //uih_loadcatalog(m_uih, "english");
    uih_newimage(m_uih);
}

struct image *AbstractFractalModel::createImage()
{
    union paletteinfo info;
    info.truec.rmask = m_redMask;
    info.truec.gmask = m_greenMask;
    info.truec.bmask = m_blueMask;

    struct palette *pal = createpalette (0, 0, TRUECOLOR, 0, 0, NULL,
                                         NULL, NULL, NULL, &info);

    struct image *img =
    create_image_cont(m_imageWidth, m_imageHeight,
                      m_bytesPerLine, 2,
                      m_buffer1,
                      m_buffer2,
                      pal, NULL,
                      0,
                      m_pixelHeight,
                      m_pixelWidth);

    return img;
}

void AbstractFractalModel::resizeImage()
{
    uih_interrupt(m_uih);
    uih_clearwindows(m_uih);
    uih_stoptimers(m_uih);
    uih_cycling_stop(m_uih);
    uih_savepalette(m_uih);
    if (m_imageWidth != m_uih->image->width || m_imageHeight != m_uih->image->height) {
        destroy_image(m_uih->image);
        destroypalette(m_uih->palette);

        if (!uih_updateimage(m_uih, createImage())) {
            //driver->uninit();
            //ui_outofmem();
            x_fatalerror(("Can not allocate tables"));
        }
        tl_process_group(syncgroup, NULL);
        //tl_reset_timer(maintimer);
        //tl_reset_timer(arrowtimer);
        uih_newimage(m_uih);
    }
    uih_newimage(m_uih);
    uih_restorepalette(m_uih);
    /*uih_mkdefaultpalette(m_uih); */
    m_uih->display = 1;
    uih_cycling_continue(m_uih);
}

const char *AbstractFractalModel::formulaName(int index)
{
}

const char *AbstractFractalModel::exteriorColorModeName(int index)
{
}

const char *AbstractFractalModel::interiorColorModeName(int index)
{
}

const char *AbstractFractalModel::planeName(int index)
{
}

const char *AbstractFractalModel::solidGuessingModeName(int index)
{
}

const char *AbstractFractalModel::dynamicResolutionModeName(int index)
{
}

const char *AbstractFractalModel::filterName(int index)
{
}


bool AbstractFractalModel::loadPosition(const char *fileName)
{
}

bool AbstractFractalModel::savePosition(const char *fileName)
{
}


bool AbstractFractalModel::replayAnimation(const char *fileName)
{
}

bool AbstractFractalModel::recordAnimation(const char *fileName)
{
}

bool AbstractFractalModel::renderAnimation(const char *fileName)
{
}


bool AbstractFractalModel::saveImage(const char *fileName)
{
}


bool AbstractFractalModel::loadRandomExample()
{
}


bool AbstractFractalModel::saveConfiguration()
{
}


void AbstractFractalModel::undo()
{
}

void AbstractFractalModel::redo()
{
}


int AbstractFractalModel::formula()
{
}

void AbstractFractalModel::setFormula(int index)
{
}


int AbstractFractalModel::exteriorColorMode()
{
}

void AbstractFractalModel::setExteriorColorMode(int index)
{
}


int AbstractFractalModel::interiorColorMode()
{
}

void AbstractFractalModel::setInteriorColorMode(int index)
{
}


void AbstractFractalModel::restoreDefaultPalette()
{
}

void AbstractFractalModel::generateRandomPalette()
{
}

int AbstractFractalModel::paletteAlgorithm()
{
}

int AbstractFractalModel::paletteSeed()
{
}

void AbstractFractalModel::generateCustomPalette()
{
}

int AbstractFractalModel::paletteCyclingSpeed()
{
}

void AbstractFractalModel::setPaletteCyclingSpeed(int speed)
{
}

int AbstractFractalModel::paletteShift()
{
}

void AbstractFractalModel::setPaletteShift(int shift)
{
}

void AbstractFractalModel::shiftPaletteForward()
{
}

void AbstractFractalModel::shiftPaletteBackward()
{
}


double AbstractFractalModel::juliaSeedX()
{
}

double AbstractFractalModel::juliaSeedY()
{
}

void AbstractFractalModel::setJuliaSeedX(double x)
{
}

void AbstractFractalModel::setJuliaSeedY(double y)
{
}


bool AbstractFractalModel::isJulia()
{
}

void AbstractFractalModel::setIsJulia(bool julia)
{
}

double AbstractFractalModel::perturbationX()
{
}

double AbstractFractalModel::perturbationY()
{
}

void AbstractFractalModel::setPerturbationX(double x)
{
}

void AbstractFractalModel::setPerturbationY(double y)
{
}


double AbstractFractalModel::centerX()
{
}

double AbstractFractalModel::centerY()
{
}

void AbstractFractalModel::setCenterX(double x)
{
}

void AbstractFractalModel::setCenterY(double y)
{
}


double AbstractFractalModel::radius()
{
}

void AbstractFractalModel::setRadius(double radius)
{
}


double AbstractFractalModel::angle()
{
}

void AbstractFractalModel::setAngle(double angle)
{
}


int AbstractFractalModel::iterations()
{
}

void AbstractFractalModel::setIterations(int iterations)
{
}


int AbstractFractalModel::bailout()
{
}

void AbstractFractalModel::setBailout(int bailout)
{
}


void AbstractFractalModel::restoreDefaultSettings()
{
}

void AbstractFractalModel::interrupt()
{
}

void AbstractFractalModel::recalculate()
{
}


int AbstractFractalModel::solidGuessingMode()
{
}

void AbstractFractalModel::setSolidGuessingMode(int mode)
{
}


int AbstractFractalModel::dynamicResolutionMode()
{
}

void AbstractFractalModel::setDynamicResolutionMode(int mode)
{
}


bool AbstractFractalModel::isPeriodicityCheckingEnabled()
{
}

void AbstractFractalModel::setPeriodicityCheckingEnabled(bool periodicity)
{
}


bool AbstractFractalModel::isFastJuliaEnabled()
{
}

void AbstractFractalModel::setFastJuliaEnabled(bool fastJulia)
{
}


bool AbstractFractalModel::isRotationEnabled()
{
}

void AbstractFractalModel::setRotationEnabled(bool rotation)
{
}


int AbstractFractalModel::rotationSpeed()
{
}

void AbstractFractalModel::setRotationSpeed(int speed)
{
}


bool AbstractFractalModel::isAutopilotEnabled()
{
}

void AbstractFractalModel::setAutopilotEnabled(bool autopilot)
{
}


int AbstractFractalModel::zoomSpeed()
{
}

void AbstractFractalModel::setZoomSpeed(int speed)
{
}


bool AbstractFractalModel::isFilterEnabled(int index)
{
}

void AbstractFractalModel::setFilterEnabled(int index, bool enabled)
{
}


double AbstractFractalModel::zoomFactor()
{
}

double AbstractFractalModel::framesPerSecond()
{
}


bool AbstractFractalModel::isFixedStepEnabled()
{
}

void AbstractFractalModel::setFixedStepEnabled(bool fixedStep)
{
}


AbstractFractalModel::MouseMode AbstractFractalModel::mouseMode()
{
}

void AbstractFractalModel::setMouseMode(AbstractFractalModel::MouseMode mouseMode)
{
}


void AbstractFractalModel::executeCommand(const char *command)
{
}

void AbstractFractalModel::clearScreen()
{
}

void AbstractFractalModel::showFractal()
{
}


void AbstractFractalModel::displayText(const char *message)
{
}


int AbstractFractalModel::textColor()
{
}

void AbstractFractalModel::setTextColor(int color)
{
}


int AbstractFractalModel::horizontalTextAlignment()
{
}

void AbstractFractalModel::setHorizontalTextAlignment(int alignment)
{
}


int AbstractFractalModel::verticalTextAlignment()
{
}

void AbstractFractalModel::setVerticalTextAlignment(int alignment)
{
}

void AbstractFractalModel::processChanges()
{

    int m_formula;
    char *m_userFormula;
    char *m_userFormulaInitialization;
    int m_interiorColorMode;
    int m_interiorTrueColorMode;
    int m_exteriorColorMode;
    int m_exteriorTrueColorMode;
    int m_paletteAlgorithm;
    int m_paletteSeed;
    int m_paletteShift;
    PaletteCyclingMode m_paletteCyclingMode;
    int m_paletteCyclingSpeed;
    double m_juliaSeedX;
    double m_juliaSeedY;
    bool m_isJulia;
    double m_perturbationX;
    double m_perturbationY;
    double m_centerX;
    double m_centerY;
    double m_radius;
    double m_angle;
    int m_iterations;
    int m_bailout;
    int m_solidGuessingMode;
    int m_dynamicResolutionMode;
    bool m_isFastJuliaEnabled;
    bool m_isRotationEnabled;
    int m_rotationSpeed;
    bool m_isAutoPilotEnabled;
    int m_zoomSpeed;
    std::vector<bool> m_filterEnabled;
    bool m_isFixedStepEnabled;
    MouseMode m_mouseMode;
    int m_textColor;
    int m_horizontalTextAlignment;
    int m_verticalTextAlignment;
}
