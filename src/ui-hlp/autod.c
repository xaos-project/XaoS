#ifndef UNSUPPORTED
static INLINE int
look1 (uih_context * context, int x, int y, int range, int max)
{
  register cpixel_t *vbuff;
  register int i, j, c = 0;
  if (range < context->zengine->image->width / 2)
    if (x < 0 || x > context->zengine->image->width ||
	y < 0 || y > context->zengine->image->height)
      return 0;
  do
    {
      max--;
      c = 0;
      if (range > context->zengine->image->width / 2)
	context->x1 =
	  rand () % (context->zengine->image->width - 2 * LOOKSIZE - 1) +
	  LOOKSIZE, context->y1 =
	  rand () % (context->zengine->image->height - 2 * LOOKSIZE - 1) +
	  LOOKSIZE;
      else
	{
	  context->x1 = rand () % range - (range >> 1) + x;
	  context->y1 = rand () % range - (range >> 1) + y;
	  if (context->x1 < LOOKSIZE)
	    context->x1 = LOOKSIZE;
	  if (context->y1 < LOOKSIZE)
	    context->y1 = LOOKSIZE;
	  if (context->x1 > context->zengine->image->width - 2 - LOOKSIZE)
	    context->x1 = context->zengine->image->width - 2 - LOOKSIZE;
	  if (context->y1 > context->zengine->image->height - 2 - LOOKSIZE)
	    context->y1 = context->zengine->image->height - 2 - LOOKSIZE;
	}
      for (j = context->y1 - LOOKSIZE; j <= context->y1 + LOOKSIZE; j++)
	{
	  vbuff = (cpixel_t *) context->zengine->image->currlines[j];
	  for (i = context->x1 - LOOKSIZE; i <= context->x1 + LOOKSIZE; i++)
	    if (InSet (p_getp (vbuff, i)))
	      c++;
	}
    }
  while ((c == 0 || c > LOOKSIZE * LOOKSIZE) && max > 0);
  if (max > 0)
    {
      context->c1 = BUTTON1, context->interlevel = 1;
      return 1;
    }
  return (0);
}
static INLINE int
look2 (uih_context * context, int x, int y, int range, int max)
{
  register cpixel_t *vbuff, *vbuff2;
  register int i, j, i1, j1, c = 0;
  if (range < context->zengine->image->width / 2)
    if (x < 0 || x > context->zengine->image->width ||
	y < 0 || y > context->zengine->image->height)
      return 0;
  do
    {
      max--;
      c = 0;

      if (range > context->zengine->image->width / 2)
	context->x1 =
	  rand () % (context->zengine->image->width - 2 * LOOKSIZE - 1) +
	  LOOKSIZE, context->y1 =
	  rand () % (context->zengine->image->height - 2 * LOOKSIZE - 1) +
	  LOOKSIZE;
      else
	{
	  context->x1 = rand () % range - (range >> 1) + x;
	  context->y1 = rand () % range - (range >> 1) + y;
	  if (context->x1 < LOOKSIZE)
	    context->x1 = LOOKSIZE;
	  if (context->y1 < LOOKSIZE)
	    context->y1 = LOOKSIZE;
	  if (context->x1 > context->zengine->image->width - 2 - LOOKSIZE)
	    context->x1 = context->zengine->image->width - 2 - LOOKSIZE;
	  if (context->y1 > context->zengine->image->height - 2 - LOOKSIZE)
	    context->y1 = context->zengine->image->height - 2 - LOOKSIZE;
	}

      for (j = context->y1 - LOOKSIZE; j < context->y1 + LOOKSIZE; j++)
	{
	  vbuff = (cpixel_t *) context->zengine->image->currlines[j];
	  for (i = context->x1 - LOOKSIZE; i <= context->x1 + LOOKSIZE; i++)
	    for (j1 = j + 1; j1 < context->y1 + LOOKSIZE; j1++)
	      {
		vbuff2 = (cpixel_t *) context->zengine->image->currlines[j1];
		for (i1 = i + 1; i1 < context->x1 + LOOKSIZE; i1++)
		  if (p_getp (vbuff, i) == p_getp (vbuff2, i1))
		    c++;
	      }
	}

    }
  while ((c > LOOKSIZE * LOOKSIZE / 2) && max > 0);
  if (max > 0)
    {
      context->c1 = BUTTON1, context->interlevel = 2;
      return 1;
    }
  return 0;
}
#endif
#undef look1
#undef look2
