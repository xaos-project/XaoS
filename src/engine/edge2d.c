#ifndef UNSUPPORTED
static void
do_edge (void *data, struct taskinfo *task, int r1, int r2)
{
  struct filter *f = (struct filter *) data;
  int y;
  unsigned int *pixels = f->image->palette->pixels;
  register unsigned int black = f->image->palette->pixels[0];
  register cpixel_t *output, *end;
  register spixel_t *up, *down, *input;

  for (y = r1; y < r2; y++)
    {
      output = p_add (((cpixel_t *) f->image->currlines[y]), 1);
      input = ((spixel_t *) f->childimage->currlines[y]) + 1;

      if (y != 0)
	up = ((spixel_t *) f->childimage->currlines[y - 1]) + 1;
      else
	up = ((spixel_t *) f->childimage->currlines[y]) + 1;

      if (y != f->image->height - 1)
	down = ((spixel_t *) f->childimage->currlines[y + 1]) + 1;
      else
	down = ((spixel_t *) f->childimage->currlines[y]) + 1;

      end =
	p_add (((cpixel_t *) f->image->currlines[y]), f->image->width - 1);
      p_setp (output, -1, 0);
      p_setp (output, f->image->width - 2, 0);

      while (output < end)
	{
	  if (input[0] > up[0] || input[0] > down[0])
	    {
	      p_set (output, pixels[input[0]]);
	    }
	  else if (input[0] != input[1])
	    {
	      if (input[0] < input[1])
		{
		  p_set (output, black);
		  p_inc (output, 1);
		  input++;
		  up++;
		  down++;
		}
	      p_set (output, pixels[input[0]]);
	    }
	  else
	    p_set (output, black);
	  p_inc (output, 1);
	  input++;
	  up++;
	  down++;
	}
    }
}
#endif
#undef do_edge
