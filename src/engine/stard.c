#ifndef UNSUPPORTED
static void
do_starfield (void *data, struct taskinfo *task, int r1, int r2)
{
  struct filter *f = (struct filter *) data;
  cpixel_t *dest;
  pixel8_t *src, *srcend;
  unsigned int color;
  int y;
  cpixeldata_t black = (cpixeldata_t) f->image->palette->pixels[0];
  mysrandom ((unsigned int) rand ());
  for (y = r1; y < r2; y++)
    {
      src = f->childimage->currlines[y];
      srcend = f->childimage->currlines[y] + f->childimage->width;
      dest = (cpixel_t *) f->image->currlines[y];
      while (src < srcend)
	{
	  color = ((unsigned int) myrandom () >> 7) & 15;
	  if (!*src
	      || (unsigned int) *src * (unsigned int) *src *
	      (unsigned int) *src >
	      (unsigned int) ((unsigned int) myrandom () & (0xffffff)))
	    {
	      p_set (dest, (cpixeldata_t) f->image->palette->pixels[color]);
	    }
	  else
	    p_set (dest, black);
	  p_inc (dest, 1);
	  src++;
	}
    }
}
#endif
#undef do_starfield
