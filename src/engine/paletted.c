#ifndef UNSUPPORTED
static void
cpalette (void *data, struct taskinfo *task, int r1, int r2)
{
  pixel8_t *src, *srcend;
  cppixel_t dest;
  struct filter *f = (struct filter *) data;
  struct palettedata *s = (struct palettedata *) f->data;
  int i;
  unsigned int *table = s->table;
  for (i = r1; i < r2; i++)
    {
      src = f->childimage->currlines[i];
      srcend = src + f->image->width;
      dest = (cppixel_t) f->image->currlines[i];
      while (src < srcend)
	{
	  p_set (dest, table[*src]);
	  src++;
	  p_inc (dest, 1);
	}
    }
}
#endif
#undef cpalette
