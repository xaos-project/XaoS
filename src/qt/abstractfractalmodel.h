#ifndef ABSTRACTFRACTALMODEL_H
#define ABSTRACTFRACTALMODEL_H

#include <vector>

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

    enum MouseMode { MouseZoomIn, MouseZoomOut, MousePan, MouseRotate, MouseJulia };
    enum HorizontalTextAlignment { TextAlignmentLeft, TextAlignmentCenter, TextAlignmentRight };
    enum VerticalTextAlignment {TextAlignmentTop, TextAlignmentMiddle, TextAlignmentBottom };
    enum PaletteCyclingMode { PaletteCyclingOff, PaletteCyclingForward, PaletteCyclingReverse };

    AbstractFractalModel();

    static void initApp(int argc, char **argv);
    static const char *formulaName(int index);
    static const char *exteriorColorModeName(int index);
    static const char *interiorColorModeName(int index);
    static const char *trueColorModeName(int index);
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

    const char *userFormula();
    void setUserFormula(const char *userFormula);

    const char *userFormulaInitialization();
    void setUserFormulaInitialization(const char *userFormulaInitialization);

    int exteriorColorMode();
    void setExteriorColorMode(int index);

    int exteriorTrueColorMode();
    void setExteriorTrueColorMode(int index);

    int interiorColorMode();
    void setInteriorColorMode(int index);

    int interiorTrueColorMode();
    void setInteriorTrueColorMode(int index);

    int paletteAlgorithm();
    void setPaletteAlgorithm(int algorithm);

    int paletteSeed();
    void setPaletteSeed(int seed);

    int paletteShift();
    void setPaletteShift(int shift);

    void shiftPaletteForward();
    void shiftPaletteBackward();

    void restoreDefaultPalette();
    void generateRandomPalette();
    void generateCustomPalette();

    PaletteCyclingMode paletteCyclingMode();
    void setPaletteCyclingMode(PaletteCyclingMode mode);

    int paletteCyclingSpeed();
    void setPaletteCyclingSpeed(int speed);

    double juliaSeedX();
    double juliaSeedY();
    void setJuliaSeedX(double x);
    void setJuliaSeedY(double y);

    bool isJulia();
    void setIsJulia(bool julia);

    double perturbationX();
    double perturbationY();
    void setPerturbationX(double x);
    void setPerturbationY(double y);

    double centerX();
    double centerY();
    void setCenterX(double x);
    void setCenterY(double y);

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
    void setPeriodicityCheckingEnabled(bool periodicityChecking);

    bool isFastJuliaEnabled();
    void setFastJuliaEnabled(bool fastJulia);

    bool isRotationEnabled();
    void setRotationEnabled(bool rotation);

    int rotationSpeed();
    void setRotationSpeed(int speed);

    bool isAutopilotEnabled();
    void setAutopilotEnabled(bool autopilot);

    int zoomSpeed();
    void setZoomSpeed(int speed);

    bool isFilterEnabled(int index);
    void setFilterEnabled(int index, bool enabled);

    bool isFixedStepEnabled();
    void setFixedStepEnabled(bool fixedStep);

    MouseMode mouseMode();
    void setMouseMode(MouseMode mouseMode);

    void executeCommand(const char *command);
    void clearScreen();
    void showFractal();

    void displayText(const char *message);

    int textColor();
    void setTextColor(int color);

    int horizontalTextAlignment();
    void setHorizontalTextAlignment(int alignment);

    int verticalTextAlignment();
    void setVerticalTextAlignment(int alignment);

    double zoomFactor();
    double framesPerSecond();

private:
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


    bool m_formulaChanged;
    bool m_userFormulaChanged;
    bool m_userFormulaInitializationChanged;
    bool m_booleriorColorModeChanged;
    bool m_booleriorTrueColorModeChanged;
    bool m_exteriorColorModeChanged;
    bool m_exteriorTrueColorModeChanged;
    bool m_paletteAlgorithmChanged;
    bool m_paletteSeedChanged;
    bool m_paletteShiftChanged;
    bool m_paletteCyclingModeChanged;
    bool m_paletteCyclingSpeedChanged;
    bool m_juliaSeedXChanged;
    bool m_juliaSeedYChanged;
    bool m_isJuliaChanged;
    bool m_perturbationXChanged;
    bool m_perturbationYChanged;
    bool m_centerXChanged;
    bool m_centerYChanged;
    bool m_radiusChanged;
    bool m_angleChanged;
    bool m_iterationsChanged;
    bool m_bailoutChanged;
    bool m_solidGuessingModeChanged;
    bool m_dynamicResolutionModeChanged;
    bool m_isFastJuliaEnabledChanged;
    bool m_isRotationEnabledChanged;
    bool m_rotationSpeedChanged;
    bool m_isAutoPilotEnabledChanged;
    bool m_zoomSpeedChanged;
    std::vector<bool> m_filterEnabledChanged;
    bool m_isFixedStepEnabledChanged;
    bool m_mouseModeChanged;
    bool m_textColorChanged;
    bool m_horizontalTextAlignmentChanged;
    bool m_verticalTextAlignmentChanged;


};

#endif // ABSTRACTFRACTALMODEL_H
