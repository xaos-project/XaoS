
static pixel32_t INLINE
calculate (number_t x, number_t y, int periodicity)
  CONSTF;
     static pixel32_t INLINE
       calculate (number_t x, number_t y, int periodicity)
{
  pixel32_t i;

  rotateback (cfractalc, x, y);
  if (cfractalc.plane)
    {
      recalculate (cfractalc.plane, &x, &y);
    }
  STAT (ncalculated2++);
#ifndef SLOWFUNCPTR
  if (cfractalc.mandelbrot)
    {
      if (cformula.flags & STARTZERO)
	i =
	  cfractalc.calculate[periodicity] (cfractalc.bre, cfractalc.bim, x,
					    y);
      else
	i =
	  cfractalc.calculate[periodicity] (x + cfractalc.bre,
					    y + cfractalc.bim, x, y);
    }
  else
    i = cfractalc.calculate[periodicity] (x, y, cfractalc.pre, cfractalc.pim);
#else
  if (cfractalc.mandelbrot)
    {
      if (cformula.flags & STARTZERO)
	i = calculateswitch (cfractalc.bre, cfractalc.bim, x, y, periodicity);
      else
	i =
	  calculateswitch (x + cfractalc.bre, y + cfractalc.bim, x, y,
			   periodicity);
    }
  else
    i = calculateswitch (x, y, cfractalc.pre, cfractalc.pim, periodicity);
#endif
  return (i);
}
