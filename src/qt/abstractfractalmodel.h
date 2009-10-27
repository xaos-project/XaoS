#ifndef ABSTRACTFRACTALMODEL_H
#define ABSTRACTFRACTALMODEL_H

class AbstractFractalModel
{
private:
    struct uih_context *m_uih;

protected:
    int m_redMask;
    int m_greenMask;
    int m_blueMask;
    int m_imageWidth;
    int m_imageHeight;
    int m_bytesPerLine;
    unsigned char *m_buffer1;
    unsigned char *m_buffer2;
    float m_pixelWidth;
    float m_pixelHeight;

    int m_mouseX;
    int m_mouseY;
    int m_mouseButtons;


private:
    struct image *createImage();
    void resizeImage();
    void createContext();

protected:
    virtual void allocateBuffers() = 0;
    virtual void processEvents() = 0;
    virtual void displayImage() = 0;

public:
    virtual void showMessage(const char *message) = 0;
    virtual void clearMessage() = 0;
    virtual void showError(const char *error) = 0;

    enum MouseMode { ZoomIn, ZoomOut, Pan, Rotate, Julia };
    enum HorizontalTextAlignment { Left, Center, Right };
    enum VerticalTextAlignment {Top, Middle, Bottom };

    AbstractFractalModel();

    static void initApp(int argc, char **argv);
    static const char *formulaName(int index);
    static const char *exteriorColorModeName(int index);
    static const char *interiorColorModeName(int index);
    static const char *planeName(int index);
    static const char *solidGuessingModeName(int index);
    static const char *dynamicResolutionModeName(int index);
    static const char *filterName(int index);

    bool loadPosition(const char *fileName);
    bool savePosition(const char *fileName);

    bool replayAnimation(const char *fileName);
    bool recordAnimation(const char *fileName);
    bool renderAnimation(const char *fileName);

    bool saveImage(const char *fileName);

    bool loadRandomExample();

    bool saveConfiguration();

    void undo();
    void redo();

    int formula();
    void setFormula(int index);

    int exteriorColorMode();
    void setExteriorColorMode(int index);

    int interiorColorMode();
    void setInteriorColorMode(int index);

    void restoreDefaultPalette();
    void generateRandomPalette();
    int customPaletteAlgorithm();
    int customPaletteSeed();
    int customPaletteShift();
    void generateCustomPalette(int algorithm, int seed, int shift);

    void cyclePaletteForward();
    void cyclePaletteReverse();
    void stopPaletteCycling();

    int paletteCyclingSpeed();
    void setPaletteCyclingSpeed(int speed);

    int paletteShift();
    void setPaletteShift(int shift);
    void shiftPaletteForward();
    void shiftPaletteBackward();

    double juliaSeedX();
    double juliaSeedY();
    void setJuliaSeedX(double x);
    void setJuliaSeedY(double y);
    void setJuliaSeed(double x, double y);

    bool isMandelbrot();
    void enableMandelbrot();
    void disableMandelbrot();

    double perturbationX();
    double perturbationY();
    void setPerturbationX(double x);
    void setPerturbationY(double y);
    void setPerturbation(double x, double y);

    bool isPerturbed();
    void enablePerturbation();
    void disablePerturbation();

    double centerX();
    double centerY();
    void setCenterX(double x);
    void setCenterY(double y);
    void setCenter(double x, double y);

    double radius();
    void setRadius(double radius);

    double angle();
    void setAngle(double angle);

    int iterations();
    void setIterations(int iterations);

    int bailout();
    void setBailout(int bailout);

    void restoreDefaultSettings();
    void interrupt();
    void recalculate();

    int solidGuessingMode();
    void setSolidGuessingMode(int mode);

    int dynamicResolutionMode();
    void setDynamicResolutionMode(int mode);

    bool isPeriodicityCheckingEnabled();
    void enablePeriodicityChecking();
    void disablePeriodicityChecking();

    bool isFastJuliaEnabled();
    void enableFastJulia();
    void disableFastJulia();

    bool isRotationEnabled();
    void enableRotation();
    void disableRotation();

    int rotationSpeed();
    void setRotationSpeed(int speed);

    bool isAutopilotEnabled();
    void enableAutopilot();
    void disableAutopilot();

    int zoomSpeed();
    void setZoomSpeed(int speed);

    void isFilterEnabled(int index);
    void enableFilter(int index);
    void disableFilter(int index);

    double zoomFactor();
    double framesPerSecond();

    int isFixedStepEnabled();
    bool enableFixedStep();
    bool disableFixedStep();

    MouseMode mouseMode();
    void setMouseMode(MouseMode mouseMode);

    void executeCommand(const char *command);
    void clearScreen();
    void showFractal();

    void displayText(const char *message);

    int textColor();
    void setTextColor(int color);

    HorizontalTextAlignment horizontalTextAlignment();
    void setHorizontalTextAlignment(HorizontalTextAlignment alignment);

    VerticalTextAlignment verticalTextAlignment();
    void setVerticalTextAlignment(VerticalTextAlignment alignment);
};

#endif // ABSTRACTFRACTALMODEL_H
