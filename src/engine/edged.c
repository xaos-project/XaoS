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
	up = ((spixel_t *) f->childimage->currlines[y - 1]) + 2;
      else
	up = ((spixel_t *) f->childimage->currlines[y]) + 2;
      if (y != f->image->height - 1)
	down = ((spixel_t *) f->childimage->currlines[y + 1]) + 2;
      else
	down = ((spixel_t *) f->childimage->currlines[y]) + 2;
      end =
	p_add (((cpixel_t *) f->image->currlines[y]), f->image->width - 1);
      p_setp (output, -1, 0);
      p_setp (output, f->image->width - 2, 0);
      while (output < end)
	{
	  if (input[1] != input[0] || input[0] != up[0]
	      || input[0] != down[0])
	    {
	      if (output < end - 2)
		{
		  p_set (output, pixels[input[0]]);
		  p_setp (output, 1, pixels[input[1]]);
		  p_setp (output, 2, pixels[input[2]]);
		  p_inc (output, 3);
		  input += 3;
		  up += 3;
		  down += 3;
		  while (output < end - 1
			 && (input[0] != up[-1] || input[0] != down[-1]))
		    {
		      p_set (output, pixels[input[0]]);
		      p_setp (output, 1, pixels[input[1]]);
		      p_inc (output, 2);
		      input += 2;
		      up += 2;
		      down += 2;
		    }
		  if (output < end
		      && (input[-1] != input[0] || up[-2] != input[0]
			  || down[-2] != input[0]))
		    {
		      p_set (output, pixels[input[0]]);
		      p_inc (output, 1);
		      input++;
		      up++;
		      down++;
		    }
		}
	      else
		p_set (output, pixels[*input]), p_inc (output, 1), input++,
		  up++, down++;
	    }
	  else
	    p_set (output, black), p_inc (output, 1), input++, up++, down++;
	}
    }
}
#endif
#undef do_edge
