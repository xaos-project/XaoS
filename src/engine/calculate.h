
static pixel32_t inline calculate(number_t x, number_t y, int periodicity);
static pixel32_t inline calculate(number_t x, number_t y, int periodicity)
{
    pixel32_t i;

    rotateback(cfractalc, x, y);
    if (cfractalc.plane) {
        recalculate(cfractalc.plane, &x, &y);
    }
    STAT(ncalculated2++);
    if (cfractalc.mandelbrot) {
        if (cformula.flags & STARTZERO)
            i = cfractalc.calculate[periodicity](cfractalc.bre, cfractalc.bim,
                                                 x, y);
        else
            i = cfractalc.calculate[periodicity](x + cfractalc.bre,
                                                 y + cfractalc.bim, x, y);
    } else
        i = cfractalc.calculate[periodicity](x, y, cfractalc.pre,
                                             cfractalc.pim);
    return (i);
}
