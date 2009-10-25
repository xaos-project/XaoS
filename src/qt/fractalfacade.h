#ifndef FRACTALFACADE_H
#define FRACTALFACADE_H

class UIAdapter;

class FractalFacade
{
private:
    const UIAdapter &ui;
    bool running;
    struct uih_context *uih;

    runLoop();

    
public:
    FractalFacade(UIAdapter &a);
    ~FractalFacade();

    void run();
    void runOnce();
    void abort();
};

#endif // FRACTALFACADE_H
