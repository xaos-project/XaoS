#ifndef UNSUPPORTED
static void
convert_3d (struct filter *f, int *x1, int *y1)
{
  struct threeddata *data = (struct threeddata *) f->data;
  int y;
  int x = *x1;
  unsigned int inp;
  unsigned int height = data->height;
  register CONST spixel_t *input;
  if (x >= f->childimage->width - 5 || x < 0 || *y1 > f->childimage->height)
    {
      *x1 += *y1 / 2;
      return;
    }
  if (x < 0)
    x = 0;
  for (y = f->childimage->height - 3; y >= 0; y--)
    {
      int d;
      input = ((spixel_t *) f->childimage->currlines[y] + y / 2);
      inp = (input[x] + input[x + 1] + input[x + 2] +
	     input[x + 3] + input[x + 4] + input[x + 5]);
      input = ((spixel_t *) f->childimage->currlines[y + 1] + y / 2);
      inp += (input[x] + input[x + 1] + input[x + 2] +
	      input[x + 3] + input[x + 4] + input[x + 5]);
      input = ((spixel_t *) f->childimage->currlines[y + 2] + y / 2);
      inp += (input[x] + input[x + 1] + input[x + 2] +
	      input[x + 3] + input[x + 4] + input[x + 5]);
      d = y - (inp / 16 > height ? height : inp / 16);
      if (d <= *y1)
	{
	  *y1 = y;
	  *x1 = x + y / 2;
	  return;
	}
    }
  *x1 += *y1 / 2;
  return;
}

static void
convertup_3d (struct filter *f, int *x1, int *y1)
{
  struct threeddata *data = (struct threeddata *) f->data;
  int y = *y1;
  int x = *x1;
  unsigned int inp;
  unsigned int height = data->height;
  register CONST spixel_t *input;
  if (x >= f->childimage->width - 5)
    x = f->childimage->width - 6;
  if (y >= f->childimage->height - 3)
    y = f->childimage->height - 3;
  if (x < 0)
    x = 0;
  if (y < 0)
    y = 0;
  input = ((spixel_t *) f->childimage->currlines[y] + y / 2);
  inp = (input[x] + input[x + 1] + input[x + 2] +
	 input[x + 3] + input[x + 4] + input[x + 5]);
  input = ((spixel_t *) f->childimage->currlines[y + 1] + y / 2);
  inp += (input[x] + input[x + 1] + input[x + 2] +
	  input[x + 3] + input[x + 4] + input[x + 5]);
  input = ((spixel_t *) f->childimage->currlines[y + 2] + y / 2);
  inp += (input[x] + input[x + 1] + input[x + 2] +
	  input[x + 3] + input[x + 4] + input[x + 5]);
  *x1 -= *y1 / 2;
  *y1 = y - (inp / 16 > height ? height : inp / 16);
}
static void
do_3d (void *dataptr, struct taskinfo *task, int r1, int r2)
{
  struct filter *RESTRICT f = (struct filter *) dataptr;
  unsigned int y;
  int maxinp = 0;
  unsigned int x;
  unsigned int end;
  unsigned int sum;
  spixel_t CONST *RESTRICT input;
  unsigned int *RESTRICT lengths;
  unsigned int *RESTRICT sums;
  unsigned int *RESTRICT relsums;
  struct threeddata *data = (struct threeddata *) f->data;


  /* Copy to local variables to improve cse and memory references.  */
  unsigned int height = data->height;
  unsigned int stereogrammode = data->stereogrammode;
  unsigned int colheight = data->colheight;
  unsigned int midcolor = data->midcolor;
  unsigned int darkcolor = data->darkcolor;
  CONST unsigned int *RESTRICT pixels = data->pixels;
  cpixel_t *RESTRICT * RESTRICT currlines =
    (cpixel_t * RESTRICT * RESTRICT) f->image->currlines;
  struct inp
  {
    int max;
    unsigned int down;
  }
   *inpdata;

#ifdef HAVE_ALLOCA1
  lengths = (int *) alloca (sizeof (int) * f->image->width);
  inpdata =
    (struct inp *) alloca (sizeof (struct inp) * (f->image->width + 2));
  sums = (int *) alloca (sizeof (int) * (f->image->width + 2) * 2);
#else
  lengths = (int *) malloc (sizeof (int) * f->image->width);
  inpdata =
    (struct inp *) malloc (sizeof (struct inp) * (f->image->width + 2));
  sums = (int *) malloc (sizeof (int) * (f->image->width + 2) * 2);
#endif
  for (x = 0; x < (unsigned int) f->image->width; x++)
    lengths[x] = f->image->height - 1,
      sums[x * 2 + 0] = 0, sums[x * 2 + 1] = 0, inpdata[x].max = 0;
  sums[x * 2 + 0] = 0, sums[x * 2 + 1] = 0, inpdata[x].max = 0;
  inpdata[x + 1].max = 0;
  end = r2;
  for (y = f->childimage->height - 2; y > 0;)
    {
      y--;
      input = ((spixel_t *) f->childimage->currlines[y] + y / 2);
      x = r1;
      relsums = sums + (y & 1);

      /* Fix boundary cases.  */
      /*relsums[0] = relsums[1];
         relsums[end*2-1] = relsums[end*2-2]; */
      inpdata[end + 1] = inpdata[end] = inpdata[end - 1];
      sum =
	input[x] + input[x + 1] + input[x + 2] + input[x + 3] + input[x + 4] +
	input[x + 5];

      while (x < end)
	{
	  unsigned int inp;
	  unsigned int d;

	  /* Average pixel values of 5*3 square to get nicer shapes.  */
	  sum += input[x + 6] - input[x];
	  inp = sum + sums[x * 2 + 1] + sums[x * 2];
	  relsums[x * 2] = sum;
	  inpdata[x].down = inp;

	  /* Calculate shades.  */
	  maxinp = inpdata[x + 2].max;
	  if ((int) inp > maxinp)
	    inpdata[x].max = inp - 32;
	  else
	    inpdata[x].max = maxinp - 32;

	  /* caluclate top of mountain.  */
	  d = inp / 16;
	  d = y - (d > height ? height : d);

	  /* Underflow */
	  if (d > 65535U)
	    d = 0;
	  if (d < lengths[x])
	    {
	      int y1;
	      unsigned int color;
	      if (stereogrammode)
		color = pixels[y];
	      else if (inp / 16 > height)
		/*Red thinks on the top.  */
		color = pixels[inp / 16 >= colheight ? colheight : inp / 16];
	      else
		{
		  int c;
		  /* Simple shading model.  
		     Depends only on the preceding voxel.  */

		  c = ((int) inpdata[x + 2].down - (int) inp) / 8;

		  /* Get shades.  */
		  color = ((int) inp > maxinp ? midcolor : darkcolor) - c;
		  color =
		    pixels[color <
			   65535 ? (color < height ? color : height) : 0];
		}
	      for (y1 = lengths[x]; y1 >= (int) d; y1--)
		{
		  p_setp (currlines[y1], x, color);
		}
	      lengths[x] = d;
	    }
	  x++;
	}
    }
#ifndef HAVE_ALLOCA1
  free (lengths);
  free (inpdata);
  free (sums);
#endif
}
#endif
#undef do_3d
#undef convert_3d
#undef convertup_3d
