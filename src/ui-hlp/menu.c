#ifndef _plan9_
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#else
#include <u.h>
#include <libc.h>
#endif

#include <aconfig.h>
#include <filter.h>
#include <fconfig.h>
#include <formulas.h>
#include <ui_helper.h>
#include <plane.h>
#include <xmenu.h>
#include "play.h"
#define LANG(name,name2) MENUSTRING("lang", NULL, name,name2,0, (void (*) (struct uih_context *c, char *))uih_loadcatalog, name2)
#define TUTOR(name1,name2,name3) MENUSTRING(name1, NULL, name2,name3,MENUFLAG_INTERRUPT|UI,uih_playtutorial,name3)

CONST static char * CONST morphstypes[] =
{
  "view",
  "julia",
  "angle",
  "line"
};
CONST static menudialog uih_smoothmorphdialog[] =
{
  DIALOGCHOICE ("Morphing type",morphstypes, 0),
  DIALOGINT ("Startuptime",0),
  DIALOGINT ("Stoptime",0),
  {NULL}
};
static void uih_smoothmorph(struct uih_context *c, dialogparam *p)
{
  if (!c->playc) return;
  switch(p[0].dint)
  {
    case 0:
      c->playc->morphtimes[0]=p[1].dint;
      c->playc->morphtimes[1]=p[2].dint;
      break;
    case 1:
      c->playc->morphjuliatimes[0]=p[1].dint;
      c->playc->morphjuliatimes[1]=p[2].dint;
      break;
    case 2:
      c->playc->morphangletimes[0]=p[1].dint;
      c->playc->morphangletimes[1]=p[2].dint;
      break;
    case 3:
      c->playc->morphlinetimes[0]=p[1].dint;
      c->playc->morphlinetimes[1]=p[2].dint;
      break;
  }
}
static CONST char * CONST imgtypes[] =
{
  "Truecolor",
  "256 colors",
  NULL
};
static CONST char * CONST yesno[] =
{
  "No",
  "Yes",
  NULL
};

CONST static menudialog uih_renderdialog[] =
{
  DIALOGIFILE ("File to render", "fract*.xaf"),
  DIALOGSTR ("basename","anim"),
  DIALOGINT ("Width",640),
  DIALOGINT ("Height",480),
  DIALOGFLOAT ("Real width (cm)",29.0),
  DIALOGFLOAT ("Real height (cm)",21.0),
  DIALOGFLOAT ("Framerate", 30),
  DIALOGCHOICE ("Image type",imgtypes, 0),
  DIALOGCHOICE ("Antialiasing",yesno,0),
  DIALOGCHOICE ("Always recalculate",yesno,0),
  DIALOGCHOICE ("Calculate MPEG motion vectors",yesno,0),
  DIALOGINT ("Recommended I frame distance",27),
  {NULL}
};
static void
uih_render (struct uih_context *c, dialogparam * d)
{
  if (d[2].dint <= 0 || d[2].dint > 4096)
    {
      uih_error (c, "renderanim:Width parameter must be positive integer in the range 0..4096");
      return;
    }
  if (d[3].dint <= 0 || d[3].dint > 4096)
    {
      uih_error (c, "renderanim:Height parameter must be positive integer in the range 0..4096");
      return;
    }
  if (d[4].number <= 0 || d[5].number <= 0)
    {
      uih_error (c, "renderanim:Invalid real width and height dimensions");
      return;
    }
  if (d[6].number <= 0 || d[6].number >= 1000000)
    {
      uih_error (c, "renderanim:invalid framerate");
      return;
    }
  if (d[7].dint && d[8].dint)
    {
      uih_error (c, "renderanim:antialiasing not supported in 256 color mode");
      return;
    }
  if (d[11].dint <= 0 || d[11].dint >= 1000000)
    {
      uih_error (c, "renderanim:incorect I frame distance");
      return;
    }
  uih_renderanimation(c,d[1].dstring, (xio_path)d[0].dstring, d[2].dint, d[3].dint,
       d[4].number/d[2].dint, d[5].number/d[3].dint, (int)(1000000/d[6].number), 
#ifdef STRUECOLOR24
       d[7].dint?C256:TRUECOLOR24,
#else
       d[7].dint?C256:TRUECOLOR,
#endif
       d[8].dint,d[9].dint, c->letterspersec, NULL, d[10].dint,d[11].dint);
}

CONST static char * CONST lineposs[] =
{
  "screen",
  "scaled",
  "fractal",
  NULL
};
CONST static menudialog uih_linedialog[] =
{
  DIALOGCHOICE ("Mode", lineposs, 0),
  DIALOGCOORD ("Start:", 0, 0),
  DIALOGCOORD ("End:", 0, 0),
  {NULL}
};
CONST char * CONST uih_colornames[] =
{
  "white",
  "black",
  "red",
  NULL
};

static menudialog uih_colordialog[] =
{
  DIALOGCHOICE ("Color", uih_colornames, 0),
  {NULL}
};
static menudialog *
uih_getcolordialog (struct uih_context *c)
{
  if (c != NULL)
    {
      uih_colordialog[0].defint = c->color;
    }
  return (uih_colordialog);
}
static void
uih_setcolor (struct uih_context *c, int color)
{
  c->color = color;
}
static CONST menudialog uih_formuladialog[] =
{
  DIALOGKEYSTR ("Formula:", "mandel"),
  {NULL}
};
static CONST menudialog uih_plviewdialog[] =
{
  DIALOGFLOAT ("X center:", 0),
  DIALOGFLOAT ("Y center:", 0),
  DIALOGFLOAT ("X Radius:", 1),
  DIALOGFLOAT ("Y Radius:", 1),
  {NULL}
};
static CONST menudialog uih_coorddialog[] =
{
  DIALOGCOORD ("Coordinates:", 0, 0),
  {NULL}
};
static CONST menudialog uih_angledialog[] =
{
  DIALOGFLOAT ("Angle:", 1),
  {NULL}
};
static CONST menudialog uih_autorotatedialog[] =
{
  DIALOGONOFF ("continuous rotation", 0),
  {NULL}
};
static CONST menudialog uih_fastrotatedialog[] =
{
  DIALOGONOFF ("Fast rotation", 0),
  {NULL}
};
static CONST menudialog uih_filterdialog[] =
{
  DIALOGKEYSTR ("filter", ""),
  DIALOGONOFF ("enable", 0),
  {NULL}
};
static menudialog uih_perturbationdialog[] =
{
  DIALOGCOORD ("Perturbation:", 0, 0),
  {NULL}
};
static menudialog *
uih_getperturbationdialog (struct uih_context *c)
{
  if (c != NULL)
    {
      uih_perturbationdialog[0].deffloat = c->fcontext->bre;
      uih_perturbationdialog[0].deffloat2 = c->fcontext->bim;
    }
  return (uih_perturbationdialog);
}
static menudialog uih_juliadialog[] =
{
  DIALOGCOORD ("Julia seed:", 0, 0),
  {NULL}
};
static menudialog *
uih_getjuliadialog (struct uih_context *c)
{
  if (c != NULL)
    {
      uih_juliadialog[0].deffloat = c->fcontext->pre;
      uih_juliadialog[0].deffloat2 = c->fcontext->pim;
    }
  return (uih_juliadialog);
}
static void
uih_plview (struct uih_context *c, dialogparam * d)
{
  if (d[2].number <= 0 || d[3].number <= 0)
    {
      uih_error (c, "animateview:Invalid viewpoint");
      return;
    }
  c->fcontext->s.cr = d[0].number;
  c->fcontext->s.ci = d[1].number;
  c->fcontext->s.rr = d[2].number;
  c->fcontext->s.ri = d[3].number;
  uih_newimage (c);
}
static void
uih_plview2 (struct uih_context *c, dialogparam * d)
{
  if (d[2].number <= 0 || d[3].number <= 0)
    {
      uih_error (c, "animateview:Invalid viewpoint");
      return;
    }
  c->fcontext->s.cr = d[0].number;
  c->fcontext->s.ci = d[1].number;
  c->fcontext->s.rr = d[2].number;
  c->fcontext->s.ri = d[3].number;
  uih_animate_image (c);
}
static menudialog uih_viewdialog[] =
{
  DIALOGCOORD ("center:", 0, 0),
  DIALOGFLOAT ("Radius:", 1),
  DIALOGFLOAT ("Angle:", 0),
  {NULL}
};
static void
uih_dview (struct uih_context *c, dialogparam * d)
{
  if (d[1].number <= 0)
    {
      uih_error (c, "Invalid viewpoint");
      return;
    }
  c->fcontext->s.cr = d[0].dcoord[0];
  c->fcontext->s.ci = d[0].dcoord[1];
  c->fcontext->s.rr = d[1].number;
  c->fcontext->s.ri = d[1].number;
  uih_angle (c, d[2].number);
  uih_newimage (c);
}
static menudialog *
uih_getviewdialog (struct uih_context *c)
{
  number_t xs, ys;
  if (c != NULL)
    {
      xs = c->fcontext->s.rr;
      ys = c->fcontext->s.ri * c->fcontext->windowwidth / c->fcontext->windowheight;
      uih_viewdialog[0].deffloat = c->fcontext->s.cr;
      uih_viewdialog[0].deffloat2 = c->fcontext->s.ci;
      uih_viewdialog[2].deffloat = c->fcontext->angle;
      if (xs > ys)
	uih_viewdialog[1].deffloat = c->fcontext->s.rr;
      else
	uih_viewdialog[1].deffloat = c->fcontext->s.ri;
    }
  return (uih_viewdialog);
}
static void
uih_printdialog (struct uih_context *c, CONST char *name)
{
  CONST menuitem *item = menu_findcommand (name);
  int y;
  CONST menudialog *di;
  if (item == NULL)
    {
      fprintf (stderr, "print_dialog:unknown function %s\n", name);
      return;
    }
  if (item->type != MENU_DIALOG && item->type != MENU_CUSTOMDIALOG)
    {
      fprintf (stderr, "print_dialog:%s don't have any dialog. Sorry.\n", name);
      return;
    }
  di = menu_getdialog (c, item);
  if (item->type == MENU_CUSTOMDIALOG)
    printf ("customdialog \"%s\"\n", item->shortname);
  else
    printf ("dialog \"%s\"\n", item->shortname);
  for (y = 0; di[y].question != NULL; y++)
    {
      printf ("dialogentry \"%s\" ", di[y].question);
      switch (di[y].type)
	{
	case DIALOG_INT:
	  printf ("integer %i", di->defint);
	  break;
	case DIALOG_FLOAT:
	  printf ("float %f", (double) di->deffloat);
	  break;
	case DIALOG_STRING:
	  printf ("string \"%s\"", di->defstr);
	case DIALOG_KEYSTRING:
	  printf ("keyword \"%s\"", di->defstr);
	  break;
	case DIALOG_IFILE:
	  printf ("inputfile \"%s\"", di->defstr);
	  break;
	case DIALOG_OFILE:
	  printf ("outputfile \"%s\"", di->defstr);
	  break;
	case DIALOG_ONOFF:
	  printf ("onoff %s", di->defint ? "#t" : "#f");
	  break;
	case DIALOG_COORD:
	  printf ("complex %f %f", (double) di->deffloat, (double) di->deffloat2);
	  break;
	case DIALOG_CHOICE:
	  printf ("choice {");
	  {
	    int y;
	    CONST char **str = (CONST char **) di->defstr;
	    for (y = 0; str[y] != NULL; y++)
	      printf ("%s ", str[y]);
	    printf ("}");
	    printf ("%s ", str[di->defint]);
	  }
	  break;
	}
      printf ("\n");
    }
  printf ("enddialog\n");
}
static menudialog printdialog[] =
{
  DIALOGSTR ("Name:", ""),
  {NULL}
};
static void
uih_printmenu (struct uih_context *c, CONST char *name, int recursive)
{
  CONST char *fullname;
  int i = 0;
  CONST menuitem *item;
  if ((fullname = menu_fullname (name)) == NULL)
    {
      printf ("Menu not found\n");
      return;
    }
  printf ("\n\nmenu \"%s\" \"%s\"\n", fullname, name);
  for (i = 0; (item = menu_item (name, i)) != NULL; i++)
    {
      if (item->type == MENU_SUBMENU)
	{
	  printf ("submenu \"%s\" \"%s\"\n", item->name, item->shortname);
	  continue;
	}
      if (item->type == MENU_SUBMENU)
        {
          printf ("separator");
          continue;
        }
      printf ("menuentry \"%s\" \"%s\" ", item->name, item->shortname);
      if (item->flags & MENUFLAG_RADIO)
	printf ("radio %s", menu_enabled (item, c) ? "on" : "off");
      else if (item->flags & MENUFLAG_CHECKBOX)
	printf ("checkbox %s", menu_enabled (item, c) ? "on" : "off");
      else
	printf ("normal");
      if (item->flags & MENUFLAG_DIALOGATDISABLE)
	printf (" dialogatdisable");
      if (item->type == MENU_DIALOG || item->type == MENU_CUSTOMDIALOG)
	printf (" dialog");
      printf ("\n");
    }
  printf ("endmenu\n");
  if (recursive)
    for (i = 0; (item = menu_item (name, i)) != NULL; i++)
      {
	if (item->type == MENU_SUBMENU)
	  {
	    uih_printmenu (c, item->shortname, 1);
	  }
      }
}
static void
uih_printmenuwr (struct uih_context *c, CONST char *name)
{
  uih_printmenu (c, name, 0);
}
static void
uih_printallmenus (struct uih_context *c)
{
  uih_printmenu (c, "root", 1);
  uih_printmenu (c, "animroot", 1);
  printf ("endmenu\n");
}
static menudialog uih_rotationdialog[] =
{
  DIALOGFLOAT ("Rotations per second:", 0),
  {NULL}
};
static menudialog uih_lettersdialog[] =
{
  DIALOGINT ("Letters per second:", 0),
  {NULL}
};
static menudialog *
uih_getlettersdialog (struct uih_context *c)
{
  if (c != NULL)
    uih_lettersdialog[0].defint = c->letterspersec;
  return (uih_lettersdialog);
}
static menudialog uih_iterdialog[] =
{
  DIALOGINT ("iterations:", 0),
  {NULL}
};
static menudialog *
uih_getiterdialog (struct uih_context *c)
{
  if (c != NULL)
    uih_iterdialog[0].defint = c->fcontext->maxiter;
  return (uih_iterdialog);
}
static CONST menudialog dtextparam[] =
{
  DIALOGSTR ("Text:", ""),
  {NULL}
};
static CONST menudialog dcommand[] =
{
  DIALOGSTR ("Your command:", ""),
  {NULL}
};
static CONST menudialog loaddialog[] =
{
  DIALOGIFILE ("Filename:", "fract*.xpf"),
  {NULL}
};
static CONST menudialog playdialog[] =
{
  DIALOGIFILE ("Filename:", "anim*.xaf"),
  {NULL}
};
static CONST menudialog saveimgdialog[] =
{
  DIALOGOFILE ("Filename:", "fract*.png"),
  {NULL}
};
static CONST menudialog saveposdialog[] =
{
  DIALOGOFILE ("Filename:", "fract*.xpf"),
  {NULL}
};
static int
uih_saveanimenabled (struct uih_context *c)
{
  if (c == NULL)
    return 0;
  return (c->save);
}
static CONST menudialog saveanimdialog[] =
{
  DIALOGOFILE ("Filename:", "anim*.xaf"),
  {NULL}
};
static CONST menudialog uih_juliamodedialog[] =
{
  DIALOGONOFF ("Julia mode:", 0),
  {NULL}
};

extern char *xtextposnames[];
extern char *ytextposnames[];
static CONST menudialog uih_textposdialog[] =
{
  DIALOGCHOICE ("horizontal position:", xtextposnames, 0),
  DIALOGCHOICE ("vertical position:", ytextposnames, 0),
  {NULL}
};
static CONST menudialog uih_fastmodedialog[] =
{
  DIALOGCHOICE ("Dynamic resolution:", save_fastmode, 0),
  {NULL}
};
static CONST menudialog uih_timedialog[] =
{
  DIALOGINT ("time:", 0),
  {NULL}
};
static CONST menudialog uih_numdialog[] =
{
  DIALOGINT ("number:", 0),
  {NULL}
};
static CONST menudialog uih_fpdialog[] =
{
  DIALOGFLOAT ("number:", 0),
  {NULL}
};

static menudialog palettedialog[] =
{
  DIALOGINT ("Algorithm number", 0),
  DIALOGINT ("seed", 0),
  DIALOGINT ("shift", 0),
  {NULL}
};
static menudialog *
uih_getrotationdialog (struct uih_context *c)
{
  if (c != NULL)
    uih_rotationdialog[0].deffloat = c->rotationspeed;
  return (uih_rotationdialog);
}
static menudialog *
uih_getpalettedialog (struct uih_context *uih)
{
  if (uih != NULL)
    {
      palettedialog[0].defint = uih->palettetype;
      palettedialog[1].defint = uih->paletteseed;
      palettedialog[2].defint = uih->paletteshift + uih->manualpaletteshift;
    }
  return (palettedialog);
}
static menudialog uih_cyclingdialog[] =
{
  DIALOGINT ("Frames per second:", 0),
  {NULL}
};
static menudialog *
uih_getcyclingdialog (struct uih_context *uih)
{
  if (uih != NULL)
    uih_cyclingdialog[0].defint = uih->cyclingspeed * uih->direction;
  return (uih_cyclingdialog);
}
static menudialog uih_speeddialog[] =
{
  DIALOGFLOAT ("Zooming speed:", 0),
  {NULL}
};
static menudialog *
uih_getspeeddialog (struct uih_context *uih)
{
  if (uih != NULL)
    uih_speeddialog[0].deffloat = uih->speedup / STEP;
  return (uih_speeddialog);
}
static void
uih_setspeed (uih_context * c, number_t p)
{
  if (p >= 100)
    p = 1.0;
  if (p < 0)
    p = 0;
  c->speedup = STEP * p;
  c->maxstep = MAXSTEP * p;

}
static void
uih_palette (struct uih_context *uih, dialogparam * p)
{
  int n1 = p[0].dint;
  int n2 = p[1].dint;
  int shift = p[2].dint;
  if (!n1)
    {
      uih_playdefpalette (uih, shift);
      return;
    }
  if (n1 < 1 || n1 > PALGORITHMS)
    {
      uih_error (uih, "Unknown palette type");
    }
  if (uih->zengine->fractalc->palette == NULL)
    return;
  if (mkpalette (uih->zengine->fractalc->palette, n2, n1 - 1) != 0)
    {
      uih_newimage (uih);
    }
  uih->manualpaletteshift = 0;
  uih->palettetype = n1;
  uih->palettechanged = 1;
  uih->paletteseed = n2;
  if (shiftpalette (uih->zengine->fractalc->palette, shift))
    {
      uih_newimage (uih);
    }
  uih->paletteshift = shift;
}

static int
uih_rotateselected (struct uih_context *c, int n)
{
  if (c == NULL)
    return 0;
  if (!c->fastrotate)
    return !n;
  return (c->rotatemode == n);
}
#if 0
static void
uih_rotate (struct uih_context *c, int n)
{
  char *names[] =
  {
    "norotate",
    "controtate",
    "mouserotate"};
  if (!n)
    uih_fastrotate (c, 0);
  else
    {
      uih_fastrotate (c, 1);
      uih_rotatemode (c, n);
    }
  uih_updatemenus (c, names[n]);
}
#endif
static int
uih_guessingselected (struct uih_context *c, int n)
{
  if (c == NULL)
    return 0;
  return (c->fcontext->range == n);
}
static int
uih_fastmode (struct uih_context *c, int n)
{
  if (c == NULL)
    return 0;
  return (c->fastmode == n);
}
static int
uih_periodicityselected (struct uih_context *c)
{
  if (c == NULL)
    return 0;
  return (c->fcontext->periodicity);
}
static void
uih_periodicitysw (struct uih_context *c)
{
  uih_setperiodicity (c, c->fcontext->periodicity ^ 1);
}
static int
uih_cyclingselected (struct uih_context *c)
{
  if (c == NULL)
    return 0;
  return (c->cycling);
}
static void
uih_cyclingsw (struct uih_context *c)
{
  c->cyclingdirection = 1;
  if (c->cycling)
    uih_cycling_off (c);
  else if (!uih_cycling_on (c))
    uih_error (c, "Inicialization of color cycling failed."),
      uih_message (c, "Try to enable palette emulation filter");
}
static void
uih_rcyclingsw (struct uih_context *c)
{
  c->cyclingdirection = -1;
  if (c->cycling)
    uih_cycling_off (c);
  else if (!uih_cycling_on (c))
    uih_error (c, "Inicialization of color cycling failed."),
      uih_message (c, "Try to enable palette emulation filter");
}
static void
uih_juliasw (struct uih_context *c)
{
  if (!c->juliamode)
    uih_enablejulia (c);
  else
    uih_disablejulia (c);
}
static int
uih_juliaselected (struct uih_context *c)
{
  if (c == NULL)
    return 0;
  return (c->juliamode);
}
static int
uih_mandelbrotselected (struct uih_context *c)
{
  if (c == NULL)
    return 0;
  return (c->fcontext->mandelbrot);
}
static void
uih_mandelbrotsw (struct uih_context *c, number_t x, number_t y)
{
  c->fcontext->mandelbrot ^= 1;
  if (c->fcontext->mandelbrot == 0 && !c->juliamode)
    {
      c->fcontext->pre = x;
      c->fcontext->pim = y;
    }
  else
    uih_disablejulia (c);
  c->fcontext->version++;
  uih_newimage (c);
  uih_updatemenus (c, "uimandelbrot");
}
static int
uih_autopilotselected (struct uih_context *c)
{
  if (c == NULL)
    return 0;
  return (c->autopilot);
}
static int
uih_fixedstepselected (struct uih_context *c)
{
  if (c == NULL)
    return 0;
  return (c->fixedstep);
}
static void
uih_persw (struct uih_context *c, number_t x, number_t y)
{
  if (c->fcontext->bre || c->fcontext->bim)
    uih_setperbutation (c, 0.0, 0.0);
  else
    uih_setperbutation (c, x, y);

}
static int
uih_perselected (struct uih_context *c)
{
  if (c == NULL)
    return 0;
  return (c->fcontext->bre || c->fcontext->bim);
}
static void
uih_autopilotsw (struct uih_context *c)
{
  if (c->autopilot)
    uih_autopilot_off (c);
  else
    uih_autopilot_on (c);
}
static void
uih_fixedstepsw (struct uih_context *c)
{
  c->fixedstep ^= 1;
}
static void
uih_setxtextpos (uih_context * c, int p)
{
  uih_settextpos (c, p, c->ytextpos);
}
static int
uih_xtextselected (uih_context * c, int p)
{
  if (c == NULL)
    return 0;
  return (c->xtextpos == p);
}
static void
uih_setytextpos (uih_context * c, int p)
{
  uih_settextpos (c, c->xtextpos, p);
}
static int
uih_ytextselected (uih_context * c, int p)
{
  if (c == NULL)
    return 0;
  return (c->ytextpos == p);
}
static void
uih_menumkpalette (uih_context * c)
{
  char s[256];
  uih_mkpalette (c);
  sprintf (s, "Algorithm:%i seed:%i size:%i", c->palettetype, c->paletteseed, c->zengine->fractalc->palette->size);
  uih_message (c, s);
}
static void
uih_shiftpalette (uih_context * c, int shift)
{
  if (shiftpalette (c->zengine->fractalc->palette, shift))
    {
      uih_newimage (c);
    }
  c->manualpaletteshift += shift;
}
static void
uih_fshift (uih_context * c)
{
  uih_shiftpalette (c, 1);
}
static void
uih_bshift (uih_context * c)
{
  uih_shiftpalette (c, -1);
}
static menudialog uih_shiftdialog[] =
{
  DIALOGINT ("Amount:", 0),
  {NULL}
};
static CONST menuitem menuitems[] =	/*XaoS menu specifications */
{

  SUBMENU ("", NULL, "Root menu", "root"),
  SUBMENU ("", NULL, "Animation root menu", "animroot"),
  SUBMENU ("", NULL, "Replay only commands", "plc"),
  SUBMENU ("", NULL, "Command line options only", "comm"),

#define MP (MENUFLAG_NOMENU|MENUFLAG_NOPLAY|MENUFLAG_ATSTARTUP)
  MENUNOP ("comm", NULL, "print menus specifications of all menus", "print_menus", MP, uih_printallmenus),
  MENUDIALOG ("comm", NULL, "print menu specification", "print_menu", MP, uih_printmenuwr, printdialog),
  MENUDIALOG ("comm", NULL, "print menu specification in xshl format", "xshl_print_menu", MP, uih_xshlprintmenu, printdialog),
  MENUNOP ("comm", NULL, "print all menu specifications in xshl format", "xshl_print_menus", MP, uih_xshlprintmenus),
  MENUDIALOG ("comm", NULL, "print dialog specification", "print_dialog", MP, uih_printdialog, printdialog),
#undef MP
#define MP (MENUFLAG_NOMENU|MENUFLAG_NOOPTION)
  /* Commands suitable only for animation replay */
  SUBMENU ("plc", NULL, "Line drawing functions", "linemenu"),
  MENUDIALOG ("linemenu", NULL, "Line", "line", MP, uih_line, uih_linedialog),
  MENUDIALOG ("linemenu", NULL, "Morph line", "morphline", MP, uih_morphline, uih_linedialog),
  MENUDIALOG ("linemenu", NULL, "Morph last line", "morphlastline", MP, uih_morphlastline, uih_linedialog),
  MENUDIALOG ("linemenu", NULL, "Set line key", "linekey", MP, uih_setkey, uih_numdialog),
  MENUNOP ("linemenu", NULL, "Clear line", "clearline", MP, uih_clear_line),
  MENUNOP ("linemenu", NULL, "Clear all lines", "clearlines", MP, uih_clear_lines),

  SUBMENU ("plc", NULL, "Animation functions", "animf"),
  MENUDIALOG ("animf", NULL, "View", "animateview", MP, uih_plview2, uih_plviewdialog),
  MENUDIALOG ("animf", NULL, "Morph view", "morphview", MP, uih_playmorph, uih_plviewdialog),
  MENUDIALOG ("animf", NULL, "Morph julia", "morphjulia", MP, uih_playmorphjulia, uih_coorddialog),
  MENUDIALOG ("animf", NULL, "Move view", "moveview", MP, uih_playmove, uih_coorddialog),
  MENUDIALOG ("animf", NULL, "Morph angle", "morphangle", MP, uih_playmorphangle, uih_angledialog),
  MENUDIALOG ("animf", NULL, "Zoom center", "zoomcenter", MP, uih_zoomcenter, uih_coorddialog),
  MENUNOP ("animf", NULL, "Zoom", "zoom", MP, uih_playzoom),
  MENUNOP ("animf", NULL, "Un-zoom", "unzoom", MP, uih_playunzoom),
  MENUNOP ("animf", NULL, "Stop zooming", "stop", MP, uih_playstop),
  MENUDIALOG ("animf", NULL, "Smooth morphing parameters", "smoothmorph", MP, uih_smoothmorph, uih_smoothmorphdialog),

  SUBMENU ("plc", NULL, "Timming functions", "time"),
  MENUDIALOG ("time", NULL, "Usleep", "usleep", MP, uih_playusleep, uih_timedialog),
  MENUNOP ("time", NULL, "Wait for text", "textsleep", MP, uih_playtextsleep),
  MENUNOP ("time", NULL, "Wait for complette image", "wait", MP, uih_playwait),
  MENUDIALOG ("plc", NULL, "Include file", "load", MP, uih_playload, loaddialog),
  MENUDIALOG ("palette", NULL, "Default palette", "defaultpalette", MP, uih_playdefpalette, uih_numdialog),
  MENUDIALOG ("fractal", NULL, "Formula", "formula", MP, uih_play_formula, uih_formuladialog),
  MENUDIALOG ("ui", NULL, "Maximal zooming step", "maxstep", MP, uih_setmaxstep, uih_fpdialog),
  MENUDIALOG ("ui", NULL, "Zooming speedup", "speedup", MP, uih_setspeedup, uih_fpdialog),
  MENUDIALOG ("mfilter", NULL, "Filter", "filter", MP, uih_playfilter, uih_filterdialog),
#undef MP
#define UI (MENUFLAG_NOPLAY|MENUFLAG_NOOPTION)


  MENUCDIALOG ("ui", NULL, "Letters per second", "letterspersec", MENUFLAG_NOMENU, uih_letterspersec, uih_getlettersdialog),
  MENUCDIALOG ("uia", NULL, "Letters per second", "letters", UI, uih_letterspersec, uih_getlettersdialog),
  MENUNOP ("uia", "z", "Interrupt", "animinterrupt", MENUFLAG_INTERRUPT | MENUFLAG_INCALC, uih_interrupt),

  SUBMENU ("root", "s", "File", "file"),
  SUBMENU ("root", NULL, "Edit", "edit"),
  SUBMENU ("root", NULL, "Fractal", "fractal"),
  SUBMENU ("root", NULL, "Calculation", "calc"),
  SUBMENU ("root", "e", "Filters", "mfilter"),
  SUBMENU ("root", NULL, "UI", "ui"),
  SUBMENU ("root", NULL, "Misc", "misc"),
  SUBMENU ("root", NULL, "Help", "helpmenu"),

  SUBMENU ("helpmenu", NULL, "Tutorials", "tutor"),

  SUBMENUNOOPT ("animroot", "f", "File", "file"),
  MENUNOP ("animroot", "s", "Stop replay", "stopreplay", UI|MENUFLAG_INTERRUPT, uih_replaydisable),
  SUBMENUNOOPT ("animroot", NULL, "Help", "helpmenu"),
  SUBMENUNOOPT ("animroot", NULL, "UI", "uia"),


  MENUDIALOG ("misc", "!", "Command", "command", UI, uih_command, dcommand),
  MENUDIALOG ("misc", NULL, "Play string", "playstr", MENUFLAG_NOMENU, uih_playstr, dcommand),
  MENUDIALOG ("misc", NULL, "Render animation", "renderanim", UI, uih_render, uih_renderdialog),
  MENUSEPARATOR ("misc"),
  MENUNOP ("misc", NULL, "Clear screen", "clearscreen", MENUFLAG_NOOPTION, uih_clearscreen),
  MENUNOP ("misc", NULL, "Display fractal", "display", MENUFLAG_NOOPTION, uih_display),
  MENUSEPARATOR ("misc"),
  MENUDIALOG ("misc", NULL, "Display text", "text", 0, uih_text, dtextparam),	/*FIXME: Should allow multiline */
  MENUCDIALOG ("misc", NULL, "Color", "color", 0, uih_setcolor, uih_getcolordialog),
  SUBMENU ("misc", NULL, "Horizontal text position", "xtextpos"),
  SUBMENU ("misc", NULL, "Vertical text position", "ytextpos"),
  MENUDIALOG ("misc", NULL, "Text position", "textposition", MENUFLAG_NOMENU | MENUFLAG_INCALC, uih_playtextpos, uih_textposdialog),
  MENUDIALOG ("misc", NULL, "Message", "message", MENUFLAG_NOMENU, uih_playmessage, dtextparam),

  MENUINTRB ("ytextpos", NULL, "Up", "ytextup", UI, uih_setytextpos, UIH_TEXTTOP, uih_ytextselected),
  MENUINTRB ("ytextpos", NULL, "Middle", "ytextmiddle", UI, uih_setytextpos, UIH_TEXTMIDDLE, uih_ytextselected),
  MENUINTRB ("ytextpos", NULL, "Bottom", "ytextbottom", UI, uih_setytextpos, UIH_TEXTBOTTOM, uih_ytextselected),
  MENUINTRB ("xtextpos", NULL, "Left", "xtextleft", UI, uih_setxtextpos, UIH_TEXTLEFT, uih_xtextselected),
  MENUINTRB ("xtextpos", NULL, "Center", "xtextcenter", UI, uih_setxtextpos, UIH_TEXTCENTER, uih_xtextselected),
  MENUINTRB ("xtextpos", NULL, "Right", "xtexteight", UI, uih_setxtextpos, UIH_TEXTRIGHT, uih_xtextselected),

  MENUDIALOG ("file", NULL, "Load", "loadpos", MENUFLAG_INTERRUPT | MENUFLAG_NOPLAY, uih_loadfile, loaddialog),
  MENUDIALOG ("file", NULL, "Save", "savepos", 0, uih_saveposfile, saveposdialog),
  MENUSEPARATOR ("file"),
  MENUDIALOGCB ("file", NULL, "Record", "record", 0, uih_saveanimfile, saveanimdialog, uih_saveanimenabled),
  MENUDIALOG ("file", NULL, "Replay", "play", MENUFLAG_INTERRUPT | MENUFLAG_NOPLAY, uih_playfile, playdialog),
  MENUSEPARATOR ("file"),
  MENUDIALOG ("file", NULL, "Save image", "saveimg", 0, uih_savepngfile, saveimgdialog),
  MENUNOP ("file", NULL, "Load random example", "loadexample", MENUFLAG_INTERRUPT, uih_loadexample),
  MENUNOP ("file", NULL, "Save configuration", "savecfg", 0, uih_savecfg),
  MENUSEPARATOR ("file"),



  MENUNOP ("edit", "u", "Undo", "undo", MENUFLAG_INTERRUPT | MENUFLAG_NOPLAY | MENUFLAG_NOOPTION, uih_undo),
  MENUNOP ("edit", NULL, "Redo", "redo",MENUFLAG_INTERRUPT | MENUFLAG_NOPLAY | MENUFLAG_NOOPTION, uih_redo),
  SUBMENU ("fractal", NULL, "formulae", "mformula"),
  MENUSEPARATOR ("fractal"),
  SUBMENU ("fractal", "f", "Incoloring mode", "mincoloring"),
  SUBMENU ("fractal", "c", "Outcoloring mode", "moutcoloring"),
  SUBMENU ("fractal", "i", "Plane", "mplane"),
  SUBMENU ("fractal", NULL, "Palette", "palettemenu"),
  MENUSEPARATOR ("fractal"),
  MENUCDIALOGCB ("fractal", "m", "Mandelbrot mode", "uimandelbrot", MENUFLAG_DIALOGATDISABLE | MENUFLAG_INTERRUPT | UI, uih_mandelbrotsw, uih_getjuliadialog, uih_mandelbrotselected),
  MENUCDIALOGCB ("fractal", "b", "Perturbation", "uiperturbation", MENUFLAG_INTERRUPT | UI, uih_persw, uih_getperturbationdialog, uih_perselected),
  MENUCDIALOG ("fractal", NULL, "Perturbation", "perturbation", MENUFLAG_NOMENU | MENUFLAG_INTERRUPT, uih_setperbutation, uih_getperturbationdialog),
  MENUSEPARATOR ("fractal"),
  MENUCDIALOG ("fractal", NULL, "View", "uiview", MENUFLAG_INTERRUPT | UI, uih_dview, uih_getviewdialog),
  MENUSEPARATOR ("fractal"),
  MENUNOP ("fractal", NULL, "Reset to defaults", "initstate", 0, uih_initstate),

  MENUDIALOG ("fractal", NULL, "Julia mode", "julia", MENUFLAG_NOMENU | MENUFLAG_INTERRUPT, uih_playjulia, uih_juliamodedialog),
  MENUDIALOG ("fractal", NULL, "View", "view", MENUFLAG_NOMENU | MENUFLAG_INTERRUPT, uih_plview, uih_plviewdialog),
  MENUDIALOG ("fractal", NULL, "Set angle", "angle", MENUFLAG_NOMENU | MENUFLAG_INTERRUPT, uih_angle, uih_angledialog),
  MENUDIALOG ("fractal", NULL, "Set plane", "plane", MENUFLAG_NOMENU | MENUFLAG_INTERRUPT, uih_setplane, uih_numdialog),
  MENUDIALOG ("fractal", NULL, "Inside coloring mode", "incoloring", MENUFLAG_NOMENU | MENUFLAG_INTERRUPT, uih_setincoloringmode, uih_numdialog),
  MENUDIALOG ("fractal", NULL, "Outside coloring mode", "outcoloring", MENUFLAG_NOMENU | MENUFLAG_INTERRUPT, uih_setoutcoloringmode, uih_numdialog),
  MENUDIALOG ("fractal", NULL, "Inside truecolor coloring mode", "intcoloring", MENUFLAG_NOMENU | MENUFLAG_INTERRUPT, uih_setintcolor, uih_numdialog),
  MENUDIALOG ("fractal", NULL, "Outside truecolor coloring mode", "outtcoloring", MENUFLAG_NOMENU | MENUFLAG_INTERRUPT, uih_setouttcolor, uih_numdialog),
  MENUDIALOG ("fractal", NULL, "Julia seed", "juliaseed", MENUFLAG_NOMENU | MENUFLAG_INTERRUPT, uih_setjuliaseed, uih_coorddialog),

  MENUNOP ("palettemenu", "d", "Default palette", "defpalette", 0, uih_mkdefaultpalette),
  MENUNOP ("palettemenu", "p", "Random palette", "randompalette", 0, uih_menumkpalette),
  MENUCDIALOG ("palettemenu", NULL, "Custom palette", "palette", 0, uih_palette, uih_getpalettedialog),
  MENUSEPARATOR ("palettemenu"),
  MENUNOPCB ("palettemenu", "y", "Color cycling", "cycling", 0, uih_cyclingsw, uih_cyclingselected),
  MENUNOPCB ("palettemenu", "Y", "Reversed color cycling", "rcycling", MENUFLAG_NOOPTION | MENUFLAG_NOPLAY, uih_rcyclingsw, uih_cyclingselected),
  MENUCDIALOG ("palettemenu", NULL, "Color cycling speed", "cyclingspeed", 0, uih_setcycling, uih_getcyclingdialog),
  MENUSEPARATOR ("palettemenu"),
  MENUDIALOG ("palettemenu", NULL, "Shift palette", "shiftpalette", 0, uih_shiftpalette, uih_shiftdialog),
  MENUNOP ("palettemenu", "+", "Shift one forward", "fshift", MENUFLAG_NOOPTION | MENUFLAG_NOPLAY, uih_fshift),
  MENUNOP ("palettemenu", "-", "Shift one backward", "bshift", MENUFLAG_NOOPTION | MENUFLAG_NOPLAY, uih_bshift),
  SUBMENU ("calc", NULL, "Solid guessing", "mguess"),
  SUBMENU ("calc", NULL, "Dynamic resolution", "dynamic"),
  MENUNOPCB ("calc", "k", "Periodicity checking", "periodicity", 0, uih_periodicitysw, uih_periodicityselected),
  MENUCDIALOG ("calc", NULL, "Iterations", "maxiter", MENUFLAG_INTERRUPT, uih_setmaxiter, uih_getiterdialog),
  MENUSEPARATOR ("calc"),
  MENUNOPCB ("calc", "j", "Fast julia mode", "fastjulia", 0, uih_juliasw, uih_juliaselected),
  SUBMENU ("calc", "o", "Rotation", "rotate"),
  MENUDIALOG ("calc", NULL, "Solid guessing range", "range", MENUFLAG_NOMENU, uih_setguessing, uih_numdialog),

  MENUINTRB ("rotate", NULL, "Disable rotation", "norotate", UI, uih_rotate, 0, uih_rotateselected),
  MENUSEPARATOR ("rotate"),
  MENUINTRB ("rotate", NULL, "Continuous rotation", "controtate", UI, uih_rotate, ROTATE_CONTINUOUS, uih_rotateselected),
  MENUINTRB ("rotate", NULL, "Rotate by mouse", "mouserotate", UI, uih_rotate, ROTATE_MOUSE, uih_rotateselected),
  MENUCDIALOG ("rotate", NULL, "Rotation speed", "rotationspeed", 0, uih_rotationspeed, uih_getrotationdialog),
  MENUDIALOG ("rotate", NULL, "Automatic rotation", "autorotate", MENUFLAG_NOMENU, uih_playautorotate, uih_autorotatedialog),
  MENUDIALOG ("rotate", NULL, "Fast rotation mode", "fastrotate", MENUFLAG_NOMENU, (funcptr) uih_fastrotate, uih_fastrotatedialog),

  MENUINTRB ("dynamic", NULL, "Disable dynamic resolution", "nodynamic", UI, uih_setfastmode, 1, uih_fastmode),
  MENUSEPARATOR ("dynamic"),
  MENUINTRB ("dynamic", NULL, "Use only during animation", "dynamicanimation", UI, uih_setfastmode, 2, uih_fastmode),
  MENUINTRB ("dynamic", NULL, "Use also for new images", "dynamicnew", UI, uih_setfastmode, 3, uih_fastmode),
  MENUDIALOG ("dynamic", NULL, "Dynamic resolution mode", "fastmode", MENUFLAG_NOMENU, uih_setfastmode, uih_fastmodedialog),

  MENUNOPCB ("ui", "a", "Autopilot", "autopilot", 0, uih_autopilotsw, uih_autopilotselected),
  MENUSEPARATOR ("ui"),
  MENUNOP ("ui", "r", "Recalculate", "recalculate", 0, uih_recalculate),
  MENUNOP ("ui", "z", "Interrupt", "interrupt", MENUFLAG_INTERRUPT | MENUFLAG_INCALC, uih_interrupt),
  MENUSEPARATOR ("ui"),
  MENUCDIALOG ("ui", NULL, "Zooming speed", "speed", 0, uih_setspeed, uih_getspeeddialog),
  MENUNOPCB ("ui", NULL, "Fixed step", "fixedstep", 0, uih_fixedstepsw, uih_fixedstepselected),

  MENUINTRB ("mguess", NULL, "Disable solid guessing", "noguess", UI, uih_setguessing, 1, uih_guessingselected),
  MENUSEPARATOR ("mguess"),
  MENUINTRB ("mguess", NULL, "Guess 2x2 rectangles", "guess2", UI, uih_setguessing, 2, uih_guessingselected),
  MENUINTRB ("mguess", NULL, "Guess 3x3 rectangles", "guess3", UI, uih_setguessing, 3, uih_guessingselected),
  MENUINTRB ("mguess", NULL, "Guess 4x4 rectangles", "guess4", UI, uih_setguessing, 4, uih_guessingselected),
  MENUINTRB ("mguess", NULL, "Guess 5x5 rectangles", "guess5", UI, uih_setguessing, 5, uih_guessingselected),
  MENUINTRB ("mguess", NULL, "Guess 6x6 rectangles", "guess6", UI, uih_setguessing, 6, uih_guessingselected),
  MENUINTRB ("mguess", NULL, "Guess 7x7 rectangles", "guess7", UI, uih_setguessing, 7, uih_guessingselected),
  MENUINTRB ("mguess", NULL, "Guess 8x8 rectangles", "guess8", UI, uih_setguessing, 8, uih_guessingselected),
  MENUINTRB ("mguess", NULL, "Guess unlimited rectangles", "guessall", UI, uih_setguessing, 2048, uih_guessingselected),


  SUBMENU ("tutor", NULL, "Language", "lang"),
  MENUSEPARATOR ("tutor"),
  SUBMENU ("tutor", NULL, "An introduction to fractals", "intro"),
  SUBMENU ("tutor", NULL, "XaoS features overview", "features"),
  SUBMENU ("tutor", NULL, "Math behind fractals", "fmath"),
  SUBMENU ("tutor", NULL, "What's new?", "new"),

  LANG ("Cesky", "cesky"),
  LANG ("Deutsch", "deutsch"),
  LANG ("English", "english"),
  LANG ("Espanhol", "espanhol"),
  LANG ("Francais", "francais"),

  TUTOR ("intro", "Whole story", "fractal.xaf"),
  MENUSEPARATOR ("intro"),
  TUTOR ("intro", "Introduction", "intro.xaf"),
  TUTOR ("intro", "Mandelbrot set", "mset.xaf"),
  TUTOR ("intro", "Julia set", "julia.xaf"),
  TUTOR ("intro", "Higher power Mandelbrots", "power.xaf"),
  TUTOR ("intro", "Newton's method", "newton.xaf"),
  TUTOR ("intro", "Barnsley's formula", "barnsley.xaf"),
  TUTOR ("intro", "Phoenix", "phoenix.xaf"),
  TUTOR ("intro", "Octo", "octo.xaf"),
  TUTOR ("intro", "Magnet", "magnet.xaf"),

  TUTOR ("features", "All features", "features.xaf"),
  MENUSEPARATOR ("features"),
  TUTOR ("features", "Outcoloring modes", "outcolor.xaf"),
  TUTOR ("features", "Incoloring modes", "incolor.xaf"),
  TUTOR ("features", "True-color coloring modes", "truecol.xaf"),
  TUTOR ("features", "Filters", "filter.xaf"),
  TUTOR ("features", "Planes", "plane.xaf"),
  TUTOR ("features", "Animations and position files", "anim.xaf"),
  TUTOR ("features", "Perturbation", "pert.xaf"),
  TUTOR ("features", "Random palettes", "palette.xaf"),
  TUTOR ("features", "Other noteworthy features", "other.xaf"),

  TUTOR ("fmath", "Whole story", "fmath.xaf"),
  MENUSEPARATOR ("fmath"),
  TUTOR ("fmath", "The definition and fractal dimension", "dimension.xaf"),
  TUTOR ("fmath", "Escape time fractals", "escape.xaf"),

  TUTOR ("new", "What's new in 3.0?", "new30.xaf"),
};
static CONST menuitem menuitems2[] =
{
  SUBMENU ("mincoloring", NULL, "True-color incoloring mode", "tincoloring"),
SUBMENU ("moutcoloring", NULL, "True-color outcoloring mode", "toutcoloring")
};
static int
uih_selectedformula (struct uih_context *c, int n)
{
  if (c == NULL)
    return 0;
  return (c->fcontext->currentformula == formulas + n);
}
static int
uih_selectedincoloring (struct uih_context *c, int n)
{
  if (c == NULL)
    return 0;
  return (c->fcontext->incoloringmode == n);
}
static void
uih_setintruecolor (struct uih_context *c, int n)
{
  uih_setincoloringmode (c, 10);
  uih_setintcolor (c, n);
}
static int
uih_selectedintcoloring (struct uih_context *c, int n)
{
  if (c == NULL)
    return 0;
  return (c->fcontext->intcolor == n);
}
static int
uih_selectedoutcoloring (struct uih_context *c, int n)
{
  if (c == NULL)
    return 0;
  return (c->fcontext->coloringmode == n);
}
static int
uih_selectedplane (struct uih_context *c, int n)
{
  if (c == NULL)
    return 0;
  return (c->fcontext->plane == n);
}
static void
uih_setouttruecolor (struct uih_context *c, int n)
{
  uih_setoutcoloringmode (c, 10);
  uih_setouttcolor (c, n);
}
static int
uih_selectedouttcoloring (struct uih_context *c, int n)
{
  if (c == NULL)
    return 0;
  return (c->fcontext->outtcolor == n);
}
static int
uih_filterenabled (struct uih_context *c, int n)
{
  if (c == NULL)
    return 0;
  return (c->filter[n] != NULL);
}
static void
uih_filtersw (struct uih_context *c, int n)
{
  if (c->filter[n] != NULL)
    uih_disablefilter (c, n);
  else
    uih_enablefilter (c, n);
}
static menuitem *formulaitems;
static menuitem *filteritems;
static char (*keys)[2];
void
uih_registermenus (void)
{
  menuitem *item;
  int i;
  menu_add (menuitems, NITEMS (menuitems));
  formulaitems = item = (menuitem *)malloc (sizeof (menuitem) * nformulas);

  /* This code automatically generates code for fractal, incoloring and other
   * menus*/
  keys = malloc (sizeof (*keys) * nformulas);
  for (i = 0; i < nformulas; i++)
    {
      item[i].menuname = "mformula";
      item[i].key = keys[i];
      keys[i][0] = '1' + i;
      if (i == 9)
	keys[i][0] = '0';
      keys[i][1] = 0;
      item[i].type = MENU_INT;
      item[i].flags = MENUFLAG_RADIO | MENUFLAG_INTERRUPT | MENUFLAG_NOPLAY;
      item[i].iparam = i;
      item[i].name = formulas[i].name[!formulas[i].mandelbrot];
      item[i].shortname = formulas[i].shortname;
      item[i].function = (void (*)(void))uih_setformula;
      item[i].control = (int (*)(void))uih_selectedformula;
    }
  menu_add (item, nformulas);

  menu_genernumbered (INCOLORING - 1, "mincoloring", incolorname, NULL, MENU_INT, UI | MENUFLAG_RADIO | MENUFLAG_INTERRUPT, uih_setincoloringmode, uih_selectedincoloring, "in");

  menu_genernumbered (TCOLOR - 1, "tincoloring", tcolorname, NULL, MENU_INT, UI | MENUFLAG_RADIO | MENUFLAG_INTERRUPT, uih_setintruecolor, uih_selectedintcoloring, "int");

  menu_genernumbered (OUTCOLORING - 1, "moutcoloring", outcolorname, NULL, MENU_INT, UI | MENUFLAG_RADIO | MENUFLAG_INTERRUPT, uih_setoutcoloringmode, uih_selectedoutcoloring, "out");

  menu_genernumbered (TCOLOR - 1, "toutcoloring", tcolorname, NULL, MENU_INT, UI | MENUFLAG_RADIO | MENUFLAG_INTERRUPT, uih_setouttruecolor, uih_selectedouttcoloring, "outt");

  {
    int i;
    for (i = 0; planename[i] != NULL; i++);
    menu_genernumbered (i, "mplane", planename, NULL, MENU_INT, UI | MENUFLAG_RADIO | MENUFLAG_INTERRUPT, uih_setplane, uih_selectedplane, "plane");
  }
  filteritems = item = (menuitem *)malloc (sizeof (menuitem) * uih_nfilters);

  /* This code automatically generates code for fractal, incoloring and other
   * menus*/
  for (i = 0; i < uih_nfilters; i++)
    {
      item[i].menuname = "mfilter";
      item[i].key = NULL;
      item[i].type = MENU_INT;
      item[i].flags = MENUFLAG_CHECKBOX | MENUFLAG_INTERRUPT | MENUFLAG_NOPLAY;
      item[i].iparam = i;
      item[i].name = uih_filters[i]->name;
      item[i].shortname = uih_filters[i]->shortname;
      if (!strcmp (item[i].shortname, "palette"))
	item[i].shortname = "palettef";
      /*this is one name collision because of ugly historical reasons */
      item[i].function = (void (*)(void))uih_filtersw;
      item[i].control = (int (*)(void))uih_filterenabled;
    }
  menu_add (item, uih_nfilters);

  menu_add (menuitems2, NITEMS (menuitems2));
}
void
uih_unregistermenus (void)
{
  menu_delete (menuitems, NITEMS (menuitems));

  free (keys);
  menu_delete (formulaitems, nformulas);
  free(formulaitems);

  menu_delnumbered (INCOLORING - 1, "in");

  menu_delnumbered (TCOLOR - 1, "int");

  menu_delnumbered (OUTCOLORING - 1, "out");

  menu_delnumbered (TCOLOR - 1, "outt");
  {
    int i;
    for (i = 0; planename[i] != NULL; i++);
    menu_delnumbered (i, "plane");
  }

  menu_delete (filteritems, uih_nfilters);
  free(filteritems);

  menu_delete (menuitems2, NITEMS (menuitems2));
}
