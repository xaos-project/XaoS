
/* 
 *     XaoS, a fast portable realtime fractal zoomer 
 *                  Copyright (C) 1996,1997 by
 *
 *      Jan Hubicka          (hubicka@paru.cas.cz)
 *      Thomas Marsh         (tmarsh@austin.ibm.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include "aconfig.h"
#ifdef DOG_DRIVER
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <allegro.h>
#include <string.h>
#include <config.h>
#include <xio.h>
#include <ui.h>
#include <timers.h>

static int width = 640, height = 480, depth = 8;
struct ui_driver dog_driver;
static int currentbuff = 0;
static BITMAP *buffers[2], *mouseb;
static int white;
static PALETTE palette;
static BITMAP *mouse_pointer;
static int mouse;
static int bill;

static unsigned char mouse_pointer_data[256] =
{
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 1, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 2, 2, 1, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 2, 2, 1, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 1, 2, 2, 2, 2, 2, 2, 1, 0,
  0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 1,
  0, 0, 0, 1, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2, 1,
  1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1,
  1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0,
  1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1,
  0, 0, 0, 1, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2, 1,
  0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 1,
  0, 0, 0, 0, 0, 0, 0, 1, 2, 2, 2, 2, 2, 2, 1, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 2, 2, 1, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 2, 2, 1, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0,
};

void 
mkwaitsprite (void)
{
  int i, col;
  mouse_pointer = create_bitmap (16, 16);

  for (i = 0; i < 256; i++)
    {
      if (bitmap_color_depth (mouse_pointer) == 8)
	{
	  switch (mouse_pointer_data[i])
	    {
	    case 1:
	      mouse_pointer->line[i / 16][i & 15] = 3;
	      break;
	    case 2:
	      mouse_pointer->line[i / 16][i & 15] = 1;
	      break;
	    default:
	      mouse_pointer->line[i / 16][i & 15] = 0;
	      break;
	    }
	}
      else
	{
	  switch (mouse_pointer_data[i])
	    {
	    case 1:
	      col = makecol (255, 255, 255);
	      break;
	    case 2:
	      col = makecol (0, 0, 0);
	      break;
	    default:
	      col = mouse_pointer->vtable->mask_color;
	      break;
	    }
	  putpixel (mouse_pointer, i & 15, i / 16, col);
	}
    }

}


/*make startup faster and executable size smaller */
int _stklen = 81920;
/*int _crt0_startup_flags = _CRT0_FLAG_LOCK_MEMORY; */
void 
__crt0_load_environment_file (char *_app_name)
{
  return;
}
/*
   void __crt0_setup_arguments(void)
   {
   return;
   } */

char **
__crt0_glob_function (char *_arg)
{
  return 0;
}






static void 
dog_print (int x, int y, CONST char *text)
{
  if (mouse)
    show_mouse (NULL);
  text_mode(0);
  textout (screen, font, text, x, y, white);
  if (mouse)
    show_mouse (mouseb);
}

static void 
dog_display ()
{
  if (mouse)
    show_mouse (NULL);
  blit (buffers[currentbuff], screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
  if (mouse)
    show_mouse (mouseb);
}


void 
set_gui_colors ()
{
  static RGB black =
  {0, 0, 0};
  static RGB grey =
  {48, 48, 48};
  static RGB white =
  {63, 63, 63};

  set_color (0, &black);
  set_color (16, &black);
  set_color (1, &grey);
  set_color (255, &white);
  gui_fg_color = 0;
  gui_bg_color = 1;
}



static void 
dog_set_color (ui_palette c, int start, int end)
{
  int i;
  for (i = start; i < end; i++)
    {
      palette[i].r = c[i - start][0] / 4;
      palette[i].g = c[i - start][1] / 4;
      palette[i].b = c[i - start][2] / 4;
    }
  set_palette (palette);
}

static void 
fix_palette ()
{
  int i;
  for (i = 0; i < 2; i++)
    {
      palette[i].r = 0;
      palette[i].g = 0;
      palette[i].b = 0;
      set_color (i, &palette[i]);
    }
  palette[255].r = 63;
  palette[255].g = 63;
  palette[255].b = 63;
  set_color (255, &palette[255]);
  set_palette (palette);

}

static void 
dog_flip_buffers ()
{
  currentbuff ^= 1;
}



static void 
dog_get_size (int *wi, int *he)
{
  static int w = 320, c = GFX_AUTODETECT, h = 200;
  int vh;
  if (mouse)
    show_mouse (NULL);
again:
  set_color_depth (8);
  set_gfx_mode (GFX_VGA, 320, 200, 0, 0);
  if ((!keypressed ()) && (!joy_b1) && (!joy_b2))
    {
      rest (500);
    }
  clear (screen);
  set_gui_colors ();
  if (!gfx_mode_select_ex (&c, &w, &h, &depth))
    {
      goto again;
    }
  vh = h;
  set_color_depth (depth);
  if (set_gfx_mode (c, w, h, 0, 0) != 0)
    {
      goto again;
    }
  width = SCREEN_W;
  height = SCREEN_H;
  switch (depth)
    {
    case 8:
      dog_driver.imagetype = UI_C256, white = 255;
      white=1;
      break;
    case 15:
      dog_driver.imagetype = UI_TRUECOLOR16, white = 0xffff;
      dog_driver.rmask = 31 * 32 * 32;
      dog_driver.gmask = 31 * 32;
      dog_driver.bmask = 31;
      break;
    case 16:
      dog_driver.imagetype = UI_TRUECOLOR16, white = 0xffff;
      dog_driver.rmask = 31 * 64 * 32;
      dog_driver.gmask = 63 * 32;
      dog_driver.bmask = 31;
      break;
    case 24:
      dog_driver.imagetype = UI_TRUECOLOR24, white = 0xffffff;
      dog_driver.rmask = 0xff0000;
      dog_driver.gmask = 0x00ff00;
      dog_driver.bmask = 0x0000ff;
      break;
    case 32:
      dog_driver.imagetype = UI_TRUECOLOR, white = 0xffffff;
      dog_driver.rmask = 0xff0000;
      dog_driver.gmask = 0x00ff00;
      dog_driver.bmask = 0x0000ff;
      break;
    }
  clear (screen);
  if (mouse)
    {
      mkwaitsprite ();
      show_mouse (screen);
    }
  if (depth == 8)
    fix_palette ();
  *wi = w;
  *he = h;
}



static void 
dog_uninitialise ()
{

  text_mode (0);
  tl_allegromode (0);
  allegro_exit ();
  install_keyboard ();
  allegro_exit ();
}


void 
dog_free (char *b1, char *b2)
{
  destroy_bitmap (buffers[0]);
  destroy_bitmap (buffers[1]);
}
int 
dog_alloc (char **b1, char **b2)
{
  buffers[0] = create_bitmap (SCREEN_W, SCREEN_H);
  if(buffers[0]==NULL) return 0;
  buffers[1] = create_bitmap (SCREEN_W, SCREEN_H);
  if(buffers[1]==NULL) return 0;
  currentbuff = 0;
  *b1 = buffers[0]->line[0];
  *b2 = buffers[1]->line[0];
  return width * ((depth + 1) / 8);
}



static void 
dog_processevents (int wait, int *x, int *y, int *b, int *k)
{
  *x = mouse_x;
  *y = mouse_y;
  *b = 0;
  while (keypressed ())
    {
      unsigned int c=readkey();
      switch(c>>8) {
      case KEY_UP:c=ui_key(UIKEY_UP);break;
      case KEY_DOWN:c=ui_key(UIKEY_DOWN);break;
      case KEY_LEFT:c=ui_key(UIKEY_LEFT);break;
      case KEY_RIGHT:c=ui_key(UIKEY_RIGHT);break;
      case KEY_ESC:c=ui_key(UIKEY_ESC);break;
      case KEY_BACKSPACE:c=ui_key(UIKEY_BACKSPACE);break;
      case KEY_HOME:c=ui_key(UIKEY_HOME);break;
      case KEY_END:c=ui_key(UIKEY_END);break;
      case KEY_PGUP:c=ui_key(UIKEY_PGUP);break;
      case KEY_PGDN:c=ui_key(UIKEY_PGDOWN);break;
      default:
      c=ui_key((unsigned char)c);
      }
      if (c == 2)
	return;
    }
  if (mouse_b & 1)
    *b |= BUTTON1;
  if (mouse_b & 2)
    *b |= BUTTON3;
  if (mouse_b & 4)
    *b |= BUTTON2;
  *k = (key[KEY_LEFT] != 0) + 2 * (key[KEY_RIGHT] != 0) + 4 * (key[KEY_UP] != 0) + 8 * (key[KEY_DOWN] != 0);

}

static void 
dog_getmouse (int *x, int *y, int *b)
{
  *x = mouse_x;
  *y = mouse_y;
  *b = 0;
  if (mouse_b == 1)
    *b |= BUTTON1;
  if (mouse_b == 2)
    *b |= BUTTON3;
  if (mouse_b & 4)
    *b |= BUTTON2;
}



int 
dog_init ()
{
  if (bill)
    i_love_bill = 1;
  i_love_bill = 1;
  allegro_init ();
  tl_allegromode (1);
  install_keyboard ();
  mouse = (install_mouse () != -1);
  if (bill)
    i_love_bill = 1;
  install_timer ();
  mouseb = screen;

  set_gfx_mode (GFX_VGA, 320, 200, 0, 0);
  signal (SIGFPE, SIG_IGN);
  fix_palette ();
  return (1);
}


static CONST struct params params[] =
{
  {"", P_HELP, NULL,"DOS driver options:"},
  {"-i_love_bill", P_SWITCH, &bill, "Enable windows friendly mode"},
  {NULL, 0, NULL, NULL}
};
void 
dog_setmouse (int mode)
{
  if (mouse)
    {
      show_mouse (NULL);
      if (mode == WAITMOUSE)
	set_mouse_sprite (mouse_pointer), set_mouse_sprite_focus (4, 6);
      else
	set_mouse_sprite (NULL), set_mouse_sprite_focus (0, 0);
      if (mode == REPLAYMOUSE)
	mouseb = NULL;
      else
	mouseb = screen;
      show_mouse (mouseb);
    }
}

struct ui_driver dog_driver =
{
  "MS-DOG",
  dog_init,
  dog_get_size,
  dog_processevents,
  dog_getmouse,
  dog_uninitialise,
  NULL,
  dog_set_color,
  dog_print,
  dog_display,
  dog_alloc,
  dog_free,
  dog_flip_buffers,
  dog_setmouse,
  NULL,
  8,
  8,
  params,
  FULLSCREEN | UPDATE_AFTER_RESIZE | PALETTE_ROTATION | ROTATE_INSIDE_CALCULATION | RESIZE_COMMAND,
  0.0, 0.0,
  0, 0,
  UI_C256,
  0, 256, 255
};

#endif
