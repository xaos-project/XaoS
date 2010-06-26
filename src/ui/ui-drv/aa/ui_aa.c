#include "aconfig.h"
#ifdef AA_DRIVER
#include <string.h>
#include <malloc.h>
#include <aalib.h>
#include <ui.h>
#include <unistd.h>
#include <xmenu.h>
#include <archaccel.h>
struct ui_driver aalib_driver;
static aa_palette palette;
static aa_palette aapalette;
static int useaapalette;
#define c context
static aa_context *c;
static unsigned char *secondary;
static int mouse;
static int nomouse;
static void aa_build_menus (void);
static void aa_remove_menus (void);
extern unsigned char *aa_chardata;
extern unsigned char *aa_colordata;
extern int aa_cursorx, aa_cursory;



static void
aa_print (int x, int y, CONST char *text)
{
  aa_puts (c, x / 2, y / 2, AA_SPECIAL, text);
}

static void
aa_display (void)
{
  int i;
  aa_renderpalette (c, useaapalette ? aapalette : palette,
		    &aa_defrenderparams, 0, 0, aa_scrwidth (c),
		    aa_scrheight (c));
  for (i = 0; i < aa_scrwidth (c) * aa_scrheight (c); i++)
    {
      if (aa_colordata[i] != 255)
	aa_text (c)[i] = aa_chardata[i], aa_attrs (c)[i] = aa_colordata[i];
    }
}
static void
aa_set_palette (ui_palette pal, int start, int end)
{
  int i;
  for (i = start; i <= end; i++)
    aa_setpalette (palette, i, pal[i - start][0], pal[i - start][1],
		   pal[i - start][2]);
  aa_display ();
  aa_flush (c);
}

static void
aa_flip_buffers (void)
{
  unsigned char *tmp;
  tmp = secondary;
  secondary = c->imagebuffer;
  c->imagebuffer = tmp;
}

static void
aa_free_buffers (char *b1, char *b2)
{
  free (aa_chardata);
  free (aa_colordata);
  free (secondary);
}

static int
aa_alloc_buffers (char **b1, char **b2, void **data)
{
  secondary = malloc (aa_imgwidth (c) * aa_imgheight (c));
  *(unsigned char **) b2 = secondary;
  *(unsigned char **) b1 = c->imagebuffer;
  aa_chardata = malloc (aa_scrwidth (c) * aa_scrheight (c));
  aa_colordata = malloc (aa_scrwidth (c) * aa_scrheight (c));
  return aa_imgwidth (c);	/* bytes per scanline */
}

static void
aa_getsize (int *w, int *h)
{
  aa_resize (c);
  *w = aa_imgwidth (c);
  *h = aa_imgheight (c);
}

static int mousex, mousey, mouseb;
static void
aa_processevents (int wait, int *mx, int *my, int *mb, int *k)
{
  int ch;
  static int keys;
  do
    {
      if ((ch = aa_getevent (c, wait)) != AA_NONE && ch != AA_MOUSE
	  && ch != AA_RESIZE && ch < 256)
	ui_key (ch);
      switch (ch)
	{
	case AA_BACKSPACE:
	  ui_key (UIKEY_BACKSPACE);
	  break;
	case AA_ESC:
	  ui_key (UIKEY_ESC);
	  break;
	case AA_RESIZE:
	  ui_resize ();
	  break;
	case AA_MOUSE:
	  aa_getmouse (c, &mousex, &mousey, &mouseb);
	  break;
	case AA_LEFT:
	  ui_key (UIKEY_LEFT);
	  if (c->kbddriver->flags & AA_SENDRELEASE)
	    keys |= 1;
	  break;
	case AA_LEFT | AA_RELEASE:
	  keys &= ~1;
	  break;
	case AA_RIGHT:
	  ui_key (UIKEY_RIGHT);
	  if (c->kbddriver->flags & AA_SENDRELEASE)
	    keys |= 2;
	  break;
	case AA_RIGHT | AA_RELEASE:
	  keys &= ~2;
	  break;
	case AA_UP:
	  ui_key (UIKEY_UP);
	  if (c->kbddriver->flags & AA_SENDRELEASE)
	    keys |= 4;
	  break;
	case AA_UP | AA_RELEASE:
	  keys &= ~4;
	  break;
	case AA_DOWN:
	  ui_key (UIKEY_DOWN);
	  if (c->kbddriver->flags & AA_SENDRELEASE)
	    keys |= 8;
	  break;
	case AA_DOWN | AA_RELEASE:
	  keys &= ~8;
	  break;
	}
      wait = 0;
    }
  while (ch != AA_NONE);
  *mx = mousex * 2;
  *my = mousey * 2;
  *k = keys;
  *mb = 0;
  if (mouseb & AA_BUTTON1)
    *mb |= BUTTON1;
  if (mouseb & AA_BUTTON2)
    *mb |= BUTTON2;
  if (mouseb & AA_BUTTON3)
    *mb |= BUTTON3;
  return;
}

#define NATTRS 7
static char *aadriver = NULL;
static char *kbddriver = NULL;
static char *mousedriver = NULL;
static char *deffont = NULL;
static int width, height, minwidth, minheight, maxwidth, maxheight, recwidth,
  recheight;
static int enable[NATTRS + 2], disable[NATTRS + 2];
static int extended, inverse, bright = 0, contrast = 0;
static float aa_gamma, dimmul, boldmul;
static int dithering[3];
static int randomval = 0;
static int masks[] =
  { AA_NORMAL_MASK, AA_DIM_MASK, AA_BOLD_MASK, AA_BOLDFONT_MASK,
  AA_REVERSE_MASK, AA_ALL, AA_EIGHT
};

#ifdef DESTICKY
extern int euid, egid;
#endif
static int
aa_initialize (void)
{
  int i, y;
  aa_parseoptions (NULL, NULL, NULL, NULL);	/*parse environment first */

  if (deffont != NULL)
    {
      for (y = 0; aa_fonts[y] != NULL; y++)
	{
	  if (!strcmp (deffont, aa_fonts[y]->name)
	      || !strcmp (deffont, aa_fonts[y]->shortname))
	    {
	      aa_defparams.font = aa_fonts[y];
	      break;
	    }
	}
    }

  if (extended)
    aa_defparams.supported |= AA_EXTENDED;

  for (i = 0; i < NATTRS; i++)
    {
      if (enable[i])
	aa_defparams.supported |= masks[i];
      if (disable[i])
	aa_defparams.supported &= ~masks[i];
    }

  for (i = 0; i < 3; i++)
    if (dithering[i])
      aa_defrenderparams.dither = i;
  if (randomval)
    aa_defrenderparams.randomval = randomval;
  if (bright)
    aa_defrenderparams.bright = bright;
  if (contrast)
    aa_defrenderparams.contrast = contrast;
  if (aa_gamma)
    aa_defrenderparams.gamma = aa_gamma;

  if (width)
    aa_defparams.width = width;
  if (height)
    aa_defparams.height = height;
  if (minwidth)
    aa_defparams.minwidth = minwidth;
  if (minheight)
    aa_defparams.minheight = minheight;
  if (maxwidth)
    aa_defparams.maxwidth = maxwidth;
  if (maxheight)
    aa_defparams.maxheight = maxheight;
  if (recwidth)
    aa_defparams.recwidth = recwidth;
  if (recheight)
    aa_defparams.recheight = recheight;

  if (aadriver != NULL)
    aa_recommendhidisplay (aadriver);
  if (kbddriver != NULL)
    aa_recommendhikbd (kbddriver);
  if (mousedriver != NULL)
    aa_recommendhimouse (mousedriver);
  if (dimmul)
    aa_defparams.dimmul = dimmul;
  if (boldmul)
    aa_defparams.boldmul = boldmul;
  if (inverse)
    aa_defrenderparams.inversion = 1;

#ifdef DESTICKY
  seteuid (euid);		/* We need supervisor rights to open mouse. */
  setegid (egid);
#endif
  c = aa_autoinit (&aa_defparams);
#ifdef DESTICKY
  seteuid (getuid ());		/* Don't need supervisor rights anymore. */
  setegid (getgid ());
#endif

  if (c == NULL)
    return 0;
  aa_autoinitkbd (c, AA_SENDRELEASE);
  if (!nomouse)
    mouse = aa_autoinitmouse (c, AA_MOUSEALLMASK);

  aalib_driver.width = aa_mmwidth (c) / 10.0;
  aalib_driver.height = aa_mmheight (c) / 10.0;
  aa_build_menus ();
  return (1);
}

static void
aa_uninitialise (void)
{
  aa_close (c);
  aa_remove_menus ();
}

static void
aa_get_mouse (int *x, int *y, int *b)
{
  if (mouse)
    aa_getmouse (c, &mousex, &mousey, &mouseb);
  *x = mousex * 2;
  *y = mousey * 2;
  *b = 0;
  if (mouseb & AA_BUTTON1)
    *b |= BUTTON1;
  if (mouseb & AA_BUTTON2)
    *b |= BUTTON2;
  if (mouseb & AA_BUTTON3)
    *b |= BUTTON3;
}

static void
aa_mousetype (int type)
{
#if (AA_LIB_VERSION>1||AA_LIB_MINNOR>0)
  if (type == REPLAYMOUSE)
    aa_hidemouse (c);
  else
    aa_showmouse (c);
#endif
}


#if AA_LIB_VERSION==1 && AA_LIB_MINNOR==0
#define SUPPORTED c->driver->params.supported
#else
#define SUPPORTED c->driverparams.supported
#endif


static int driver;
static int font;
static int mask;

static void
aa_save5 (void)
{
  struct aa_hardware_params p = aa_defparams;
  struct aa_context *sc;
  struct aa_savedata data;
  int i;
  char *name;
  char extension[100];
  if (aa_formats[driver]->flags & AA_USE_PAGES)
    strcpy (extension, "_0_0");
  else
    extension[0] = 0;
  strcat (extension, aa_formats[driver]->extension);

#ifdef DJGPP
  name = ui_getfile ("fr", extension);
  /*fit into ugly 8char. limit */
#else
  name = ui_getfile ("fract", extension);
#endif

  if (name == NULL)
    {
      aa_print (0, 0, "Save driver initialization failed");
      aa_flush (c);
      sleep (3);
      return;
    }
  for (i = 0; name[i] != '.' && name[i] != '_'; i++);
  name[i] = 0;
  strcat (name, "%c%e");

  p.minwidth = p.maxwidth = 0;
  p.minheight = p.maxheight = 0;
  p.width = aa_scrwidth (c);
  p.height = aa_scrheight (c);
  p.supported = mask;
  if (aa_formats[driver]->font == NULL)
    p.font = aa_fonts[font];

  data.format = aa_formats[driver];
  data.name = name;
  sc = aa_init (&save_d, &p, &data);
  if (sc == NULL)
    {
      aa_print (0, 0, "Save driver initialization failed");
      aa_flush (c);
      sleep (3);
      return;
    }
  memcpy (sc->imagebuffer, c->imagebuffer,
	  (aa_imgwidth (c) - 1) * aa_imgheight (c));
  aa_renderpalette (sc, useaapalette ? aapalette : palette,
		    &aa_defrenderparams, 0, 0, aa_scrwidth (c),
		    aa_scrheight (c));
  aa_flush (sc);
  aa_close (sc);
}

static void
aa_swinversion (struct uih_context *c)
{
  aa_defrenderparams.inversion ^= 1;
}
static int
aa_inversion (struct uih_context *c)
{
  return (aa_defrenderparams.inversion);
}
static int aacurpalette = 0;
static void
aa_setXpalette (struct uih_context *c, int m)
{
  int i, s;
  if (!m)
    {
      useaapalette = 0;
      aa_display ();
      return;
    }
  useaapalette = 1;
  aacurpalette = m;
  s = 1 << m;
  s--;
  for (i = 0; i < 255; i++)
    {
      aapalette[i] = (i & (s)) * 255 / s;
    }
}
static int
aa_getpalette (struct uih_context *c, int m)
{
  if (!useaapalette)
    return (!m);
  return (m == aacurpalette);
}
static void
aa_dither (struct uih_context *c, int m)
{
  aa_defrenderparams.dither = m;
}
static int
aa_getdither (struct uih_context *c, int mode)
{
  return ((int) aa_defrenderparams.dither == (int) mode);
}


static void
aa_sfont (struct uih_context *co, int i)
{
  aa_setfont (c, aa_fonts[i]);
}
CONST static char *CONST name[] = {
  "normal characters     ",
  "half bright(dim)      ",
  "double bright(bold)   ",
  "bold font             ",
  "reversed              ",
  "reserved characters   ",
  "non ascii characters  ",
  "leave menu",
};
CONST static char *CONST ttext[] = {
  "Use XaoS palette",
  "Black and white stripes",
  "4 gray palette",
  "16 gray palette",
  "32 gray palette",
  "64 gray palette",
  "128 gray palette",
  "256 gray palette"
};
static CONST int attribs[] = {
  AA_NORMAL_MASK,
  AA_DIM_MASK,
  AA_BOLD_MASK,
  AA_BOLDFONT_MASK,
  AA_REVERSE_MASK,
  AA_ALL,
  AA_EIGHT,
  0
};

#define MYNATTRS (sizeof(name)/sizeof(char *))
static void
aa_swattr (struct uih_context *co, int m)
{
  int mask;
  if (m < (int) MYNATTRS - 1)
    {
      mask = c->params.supported;
      mask ^= attribs[m];
      aa_setsupported (c, mask);
      ui_menu ("aa_attr");
    }
}
static int
aa_getattr (struct uih_context *co, int m)
{
  return (c->params.supported & attribs[m]);
}
static int
aa_getsave3 (struct uih_context *c, int m)
{
  return (mask & attribs[m]);
}
static void
aa_save3 (struct uih_context *co, int m)
{
  if (m < (int) MYNATTRS - 1)
    {
      mask ^= attribs[m];
      mask &= aa_formats[driver]->supported;
      ui_menu ("aa_save3");
    }
  else
    aa_save5 ();
}
static void
aa_save2 (struct uih_context *cc, int m)
{
  aa_display ();
  font = m;
  mask = aa_formats[driver]->supported & c->params.supported;
  if (!mask)
    mask = aa_formats[driver]->supported;
  ui_menu ("aa_save3");
}
static void
aa_save (struct uih_context *c, int m)
{
  driver = m;
  if (aa_formats[m]->font != NULL)
    aa_save3 (NULL, 0);
  else
    ui_menu ("aa_save2");
}

#define UI (MENUFLAG_NOOPTION|MENUFLAG_NOPLAY)
static CONST menuitem menuitems[] = {
  SUBMENU ("file", NULL, "Save as text file", "aa_format"),
  SUBMENU ("ui", NULL, "Attributes", "aa_attr"),
  SUBMENU ("ui", NULL, "Font", "aa_font"),
  MENUNOPCB ("ui", NULL, "Inversion", "aainversion", UI, aa_swinversion,
	     aa_inversion),
  SUBMENU ("ui", NULL, "Dithering mode", "aa_dithering"),
  SUBMENU ("", NULL, "Font for saved file", "aa_save2"),
  SUBMENU ("", NULL, "Save attributes", "aa_save3"),
  SUBMENU ("palettemenu", NULL, "Text palette", "aa_palette"),
};

static menuitem *fontmenus;
static menuitem *fontmenus2;
static menuitem *formatmenus;
static int nfonts, nformats;
static void
aa_build_menus ()
{
  int i;
  menu_add (menuitems, NITEMS (menuitems));
  menu_genernumbered (sizeof (ttext) / sizeof (char *), "aa_palette", ttext,
		      NULL, MENU_INT, MENUFLAG_RADIO, aa_setXpalette,
		      aa_getpalette, "palettemenu");
  menu_genernumbered (AA_DITHERTYPES, "aa_dithering",
		      (CONST char *CONST * CONST) aa_dithernames, NULL,
		      MENU_INT, MENUFLAG_RADIO, aa_dither, aa_getdither,
		      "dither");
  menu_genernumbered (MYNATTRS, "aa_attr", name, NULL, MENU_INT,
		      MENUFLAG_RADIO, aa_swattr, aa_getattr, "attribute");
  menu_genernumbered (MYNATTRS, "aa_save3", name, NULL, MENU_INT,
		      MENUFLAG_RADIO, aa_save3, aa_getsave3, "save");
  for (i = 0; aa_fonts[i] != NULL; i++);
  nfonts = i;
  fontmenus = malloc (sizeof (menuitem) * i);
  fontmenus2 = malloc (sizeof (menuitem) * i);
  for (i = 0; aa_fonts[i] != NULL; i++)
    {
      char s[256];
      sprintf (s, "font%i%s", i, aa_fonts[i]->shortname);
      fontmenus[i].name = aa_fonts[i]->name;
      fontmenus[i].shortname = strdup (s);
      fontmenus[i].menuname = "aa_font";
      fontmenus[i].key = NULL;
      fontmenus[i].type = MENU_INT;
      fontmenus[i].flags = UI;
      fontmenus[i].iparam = i;
      fontmenus[i].function = (void (*)(void)) aa_sfont;
      fontmenus2[i].name = aa_fonts[i]->name;
      fontmenus2[i].shortname = fontmenus[i].shortname;
      fontmenus2[i].menuname = "aa_save2";
      fontmenus2[i].key = NULL;
      fontmenus2[i].type = MENU_INT;
      fontmenus2[i].flags = UI;
      fontmenus2[i].iparam = i;
      fontmenus2[i].function = (void (*)(void)) aa_save2;
    }
  menu_add (fontmenus, nfonts);
  menu_add (fontmenus2, nfonts);
  for (i = 0; aa_formats[i] != NULL; i++);
  nformats = i;
  formatmenus = malloc (sizeof (menuitem) * i);
  for (i = 0; aa_formats[i] != NULL; i++)
    {
      formatmenus[i].name = aa_formats[i]->formatname;
      formatmenus[i].shortname = aa_formats[i]->formatname;
      formatmenus[i].menuname = "aa_format";
      formatmenus[i].key = NULL;
      formatmenus[i].type = MENU_INT;
      formatmenus[i].flags = UI;
      formatmenus[i].iparam = i;
      formatmenus[i].function = (void (*)(void)) aa_save;
    }
  menu_add (formatmenus, nformats);
}
static void
aa_remove_menus ()
{
  int i;
  menu_delnumbered (sizeof (ttext) / sizeof (char *), "palettemenu");
  menu_delnumbered (AA_DITHERTYPES, "dither");
  menu_delnumbered (MYNATTRS, "attribute");
  menu_delnumbered (MYNATTRS, "save");
  menu_delete (menuitems, NITEMS (menuitems));
  menu_delete (fontmenus, nfonts);
  menu_delete (fontmenus2, nfonts);
  free (fontmenus2);
  menu_delete (formatmenus, nformats);
  free (formatmenus);
  for (i = 0; aa_fonts[i] != NULL; i++)
    {
      free ((char *) fontmenus[i].shortname);
    }
  free (fontmenus);
}
static int cursorvisible = 1;
static void
aa_fflush (void)
{
  if (aa_cursorx < 0)
    {
      if (cursorvisible)
	{
	  aa_gotoxy (c, 0, 0);
	  aa_hidecursor (c);
	  cursorvisible = 0;
	}
    }
  else
    {
      aa_gotoxy (c, aa_cursorx, aa_cursory);
      if (!cursorvisible)
	aa_showcursor (c), cursorvisible = 1;
    }
  aa_flush (c);
}

static CONST struct params params[] = {
  {"", P_HELP, NULL, "AA driver options:"},
  {"-aadriver", P_STRING, &aadriver, "Select display driver used by aa-lib"},
  {"-kbddriver", P_STRING, &kbddriver,
   "Select keyboard driver used by aa-lib"},
  {"-mousedriver", P_STRING, &mousedriver,
   "Select keyboard driver used by aa-lib"},
  {"-font", P_STRING, &deffont, "Select font"},
  {"-width", P_NUMBER, &width, "Set width"},
  {"-height", P_NUMBER, &height, "Set height"},
  {"-minwidth", P_NUMBER, &minwidth, "Set minimal allowed width"},
  {"-minheight", P_NUMBER, &minheight, "Set minimal allowed height"},
  {"-maxwidth", P_NUMBER, &maxwidth, "Set maximal allowed width"},
  {"-maxheight", P_NUMBER, &maxheight, "Set maximal allowed height"},
  {"-recwidth", P_NUMBER, &recwidth, "Set recommended width"},
  {"-recheight", P_NUMBER, &recheight, "Set recommended height"},
  {"-normal", P_SWITCH, enable, "enable usage of narmal characters"},
  {"-nonormal", P_SWITCH, disable, "disable usage of narmal characters"},
  {"-dim", P_SWITCH, enable + 1,
   "enable usage of dim(half bright) characters"},
  {"-nodim", P_SWITCH, disable + 1,
   "disable usage of dim(half bright) characters"},
  {"-bold", P_SWITCH, enable + 2,
   "enable usage of bold(double bright) characters"},
  {"-nobold", P_SWITCH, disable + 2,
   "disable usage of bold(double bright) characters"},
  {"-boldfont", P_SWITCH, enable + 3, "enable usage of boldfont characters"},
  {"-noboldfont", P_SWITCH, disable + 3,
   "disable usage of boldfont characters"},
  {"-reverse", P_SWITCH, enable + 4, "enable usage of reversed characters"},
  {"-noreverse", P_SWITCH, disable + 4,
   "disable usage of reversed characters"},
  {"-all", P_SWITCH, enable + 5, "enable usage of reserved characters"},
  {"-eight", P_SWITCH, enable + 6, "enable usage of non ansii characters"},
  {"-extended", P_SWITCH, &extended,
   "enable usage of extended character set"},
  {"-inverse", P_SWITCH, &inverse, "enable inverse"},
  {"-bright", P_NUMBER, &bright, "set bright (0-255)"},
  {"-contrast", P_NUMBER, &contrast, "set contrast (0-255)"},
  {"-gamma", P_FLOAT, &aa_gamma, "set famma (0-1)"},
  {"-nodither", P_SWITCH, dithering, "Disable dithering"},
  {"-floyd_steinberg", P_SWITCH, dithering + 2,
   "Enable floyd steinberg dithering"},
  {"-error_distribution", P_SWITCH, dithering + 1,
   "Enable error distribution dithering"},
  {"-random", P_NUMBER, &randomval, "Set random dithering value"},
  {"-dimmul", P_FLOAT, &dimmul, "Multiply factor for dim color (5.3)"},
  {"-boldmul", P_FLOAT, &boldmul, "Multiply factor for bold color (5.3)"},
  {"-nomouse", P_SWITCH, &nomouse, "Disable mouse"},
  {NULL, 0, NULL, NULL}
};

struct ui_driver aalib_driver = {
  "aa",
  aa_initialize,
  aa_getsize,
  aa_processevents,
  aa_get_mouse,
  aa_uninitialise,
  NULL,				/*You should implement just one */
  aa_set_palette,		/*of these and add NULL as second */
  aa_print,
  aa_display,
  aa_alloc_buffers,
  aa_free_buffers,
  aa_flip_buffers,
  aa_mousetype,
  aa_fflush,
  2,				/*text width */
  2,				/*text height */
  params,
  PALETTE_REDISPLAYS | SCREENSIZE | UPDATE_AFTER_RESIZE | AALIB,	/*flags...see ui.h */
  0.0, 0.0,			/*width/height of screen in centimeters */
  0, 0,				/*resolution of screen for windowed systems */
  UI_C256,			/*Image type */
  0, 255, 255			/*start, end of palette and maximum allocatable */
    /*entries */
};

#endif
