#ifndef UNSUPPORTED
static void
do_stereogram (void *data, struct taskinfo *task, int r1, int r2)
{
  struct filter *f = (struct filter *) data;
  int i, y, lc;
  struct stereogramdata *s = (struct stereogramdata *) f->data;
  register cpixel_t *cs, *c, *src, *src1, *ce;
  register spixel_t *c1;
  unsigned int *pixels = f->image->palette->pixels;
  s->minc = NCOLORS;
  for (i = r1; i < r2; i++)
    {
      int i1;
      for (i1 = 0; i1 < 2; i1++)
	{
	  c1 = (spixel_t *) f->childimage->currlines[i];
	  c = cs = (cpixel_t *) f->image->currlines[2 * i + i1];
	  ce = p_add (cs, f->image->width);
	  src = src1 = c;
	  lc = 1024;
	  while (c < ce)
	    {
	      y = *c1;
	      if (y == lc)
		p_inc (src, 2);
	      else
		{
		  lc = y;
		  if (y < s->minc && y != 0)
		    s->minc = y;
		  y = table[y];
		  src = p_add (c, -y);
		}
	      if (src < src1)
		{
		  p_set (c, pixels[(rand () & 15)]);
		  p_setp (c, 1, pixels[(rand () & 15)]);
		}
	      else
		{
		  if (src <= cs)
		    {
		      p_set (c, pixels[(rand () & 15)]);
		      p_setp (c, 1, pixels[(rand () & 15)]);
		    }
		  else
		    {
		      p_copy (c, 0, src, 0);
		      p_copy (c, 1, src, 1);
		    }
		  src1 = src;
		}
	      p_inc (c, 2);
	      c1++;
	    }
	}
    }
}
#endif
#undef do_stereogram
