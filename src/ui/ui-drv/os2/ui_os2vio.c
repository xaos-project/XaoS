/* 
 *     XaoS, a fast portable realtime fractal zoomer 
 *                  Copyright (C) 1996,1997 by
 *
 *      Jan Hubicka          (hubicka@paru.cas.cz)
 *      Thomas Marsh         (tmarsh@austin.ibm.com)
 *
 *     OS/2 VIO driver by
 *      Thomas A. K. Kjaer   (takjaer@imv.aau.dk)
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
#ifdef OS2VIO_DRIVER
#define INCL_VIO
#define INCL_KBD
#define INCL_MOU
#include <os2.h>
#include <sys/hw.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "ui.h"
#include "ui_os2viofont.h"

struct ui_driver os2vio_driver;

static int currentbuff = 0;
static int width=320, height=200;

static int page_size = 64000;

static int buttons = 0, keys = 0;
static char *buffers[2] = {NULL, NULL};

static PVIOPHYSBUF phys_buf;
static VIOMODEINFO orig_mode, vio_mode;
static PCH ScrPtr;

static HMOU hMou;
static USHORT mouwait = MOU_NOWAIT;
static USHORT oldDeviceStatus;
static int mousetype=NORMALMOUSE;
static char screen[8*8];

static void vio_mousetype(int m)
{
  mousetype = m;
}

static int vio_init()
{
  USHORT mask = 0x7F;
  USHORT status;
  PTRLOC ptrLoc = {0, 0};

  if (MouOpen(NULL,&hMou) && MouOpen("POINTER$", &hMou))
    hMou = 0;

  MouFlushQue(hMou);
  MouSetEventMask(&mask, hMou);

  MouGetDevStatus(&oldDeviceStatus, hMou);

  /* Turn off mouse pointer drawing, BEFORE switching to graphics mode! */
  status = 0x100;
  MouSetDevStatus(&status, hMou);

  orig_mode.cb=sizeof(VIOMODEINFO);
  VioGetMode((PVIOMODEINFO) &orig_mode,(HVIO) 0);
  MouSetPtrPos(&ptrLoc, hMou);

  memcpy((void *) &vio_mode, (const void *) &orig_mode, sizeof(VIOMODEINFO));
  vio_mode.fbType = 3;
  vio_mode.color = 8;
  vio_mode.col = 40;
  vio_mode.row = 25;
  vio_mode.hres = width;
  vio_mode.vres = height;
  vio_mode.fmt_ID = 0;
  vio_mode.attrib = 0;

  if (VioSetMode(&vio_mode, (HVIO) 0)) {
    printf("VioSetMode error\n");
    return 0;
  }

  phys_buf = malloc(sizeof(VIOPHYSBUF));
  phys_buf->pBuf=(unsigned char *) 0xA0000;
  phys_buf->cb=page_size;
  if (VioGetPhysBuf(phys_buf, 0)) {
    printf("VioGetPhysBuf error\n");
    return 0;
  }

  ScrPtr=MAKEP(phys_buf->asel[0],0);
  memset(ScrPtr, 0, page_size);

  return 1;
}

static void vio_get_size(int *wi, int *he)
{
  *wi = width; *he = height;
}

#define ABS(a) (((a)<0) ? -(a) : (a))

static int vga_drawline(int x1, int y1, int x2, int y2, int c)
{
  int dx = x2 - x1;
  int dy = y2 - y1;
  int ax = ABS(dx) << 1;
  int ay = ABS(dy) << 1;
  int sx = (dx >= 0) ? 1 : -1;
  int sy = (dy >= 0) ? 1 : -1;
  
  int x = x1;
  int y = y1;
  
  if (ax > ay) {
    int d = ay - (ax >> 1);
    while (x != x2) {
      ScrPtr[y * width + x] = c;
      
      if (d > 0 || (d == 0 && sx == 1)) {
	y += sy;
	d -= ax;
      }
      x += sx;
      d += ay;
    }
  } else {
    int d = ax - (ay >> 1);
    while (y != y2) {
      ScrPtr[y * width + x] = c;
      
      if (d > 0 || (d == 0 && sy == 1)) {
	x += sx;
	d -= ay;
      }
      y += sy;
      d += ax;
    }
  }
  ScrPtr[y * width + x] = c;
  
  return 0;
  
}

static void draw_mouse(int x, int y, int clear)
{
  static int oldx=3, oldy=3;
  int buffstart, i;
  char status;

  if(mousetype!=REPLAYMOUSE) {
    VioScrLock(1, &status, (HVIO) 0);
    if (x < 3)
      x = 3;
    if (y < 3)
      y = 3;
    if (x > width - 5)
      x = width - 5;
    if (y > height - 4)
      y = height - 4;
    
    if ((oldx != x || oldy != y) && clear) {
      /* copy rectangle over mouse pointer */
      buffstart = (oldy - 3) * width + (oldx - 3);
      for (i = 0; i < 8; i++) {
	memcpy((PCH)(ScrPtr+buffstart),
	       (char *)(buffstart+buffers[currentbuff]), 8);
	buffstart += width;
      }
    }
    
    vga_drawline(x - 3, y - 3, x + 3, y + 3, 255);
    vga_drawline(x - 3, y + 3, x + 3, y - 3, 255);
    vga_drawline(x + 1 - 3, y - 3, x + 1 + 3, y + 3, 253);
    vga_drawline(x + 1 - 3, y + 3, x + 1 + 3, y - 3, 253);
    
    oldx = x;
    oldy = y;
    VioScrUnLock((HVIO)0);
  }
}

/* check for keys
 *
 * returns ascii-code
 */
static int check_keys(KBDKEYINFO k)
{
  if(!(k.fbStatus&0x02)) { /* ASCII code */
    if(k.chChar==27) return 'q';

    return k.chChar;
  }

  return 0;  /* not handled */
}

static int check_ckeys(KBDKEYINFO k)
{
  int c;

  if(k.fbStatus&0x02) { /* SCAN code */
    c = k.chScan;

    if (c == 75) { /* left */
      keys |= 1;
    } else {
      keys &= ~1;
    }
    if (c == 77) { /* right */
      keys |= 2;
    } else {
      keys &= ~2;
    }
    if (c == 72) { /* up */
      keys |= 4;
    } else {
      keys &= ~4;
    }
    if (c == 80) { /* down */
      keys |= 8;
    } else {
      keys &= ~8;
    }
  }
  return keys;
}


static void vio_processevents(int wait, int *x, int *y, int *b, int *k)
{
  KBDKEYINFO kbdkeyinfo;
  MOUEVENTINFO mouEvent;

  *k = 0;
  *b = buttons;
  if (wait) {
    while(1) {
      KbdCharIn(&kbdkeyinfo, IO_NOWAIT, 0);
      if (kbdkeyinfo.fbStatus!=0) {
	if(check_keys(kbdkeyinfo))
	  ui_key(kbdkeyinfo.chChar);
        *k = check_ckeys(kbdkeyinfo);
        return;
      }
      MouReadEventQue(&mouEvent, &mouwait, hMou);
      if(!((mouEvent.fs==0)&&(mouEvent.col==0)&&(mouEvent.row==0))) {
	if(mouEvent.fs&0x60)
	  buttons |= BUTTON2;
	else
	  buttons &= ~BUTTON2;
	if((mouEvent.fs&0x18)||(mouEvent.fs&0x60 && mouEvent.fs&0x06))
	  buttons |= BUTTON3;
	else
	  buttons &= ~BUTTON3;
	if(mouEvent.fs&0x06)
	  buttons |= BUTTON1;
	else
	  buttons &= ~BUTTON1;
	if(mouEvent.fs&0x01)
	  buttons = 0;
	*x=mouEvent.col;
	*y=mouEvent.row;
	*b=buttons;

        draw_mouse(*x, *y, 1);
        return;
      }
      DosSleep(0);
    }
  } else {
    KbdCharIn(&kbdkeyinfo, IO_NOWAIT, 0);
    if (kbdkeyinfo.fbStatus!=0) {
      if(check_keys(kbdkeyinfo))
  	ui_key(kbdkeyinfo.chChar);
      *k = check_ckeys(kbdkeyinfo);
    }
    os2vio_driver.getmouse(x, y, b);
  }
  return;
}

static void vio_getmouse(int *x, int *y, int *b)
{
  PTRLOC ptrLoc;
  MOUEVENTINFO mouEvent;

  MouGetPtrPos(&ptrLoc, hMou);
  MouReadEventQue(&mouEvent, &mouwait, hMou);
  *x = ptrLoc.col; *y = ptrLoc.row;
  if((mouEvent.fs==0)&&(mouEvent.col==0)&&(mouEvent.row==0)) { /* no event */
    *b = buttons;
    draw_mouse(ptrLoc.col, ptrLoc.row, 1);
    return;
  }
  if(mouEvent.fs&0x60)
    buttons |= BUTTON2;
  else
    buttons &= ~BUTTON2;
  if((mouEvent.fs&0x18)||(mouEvent.fs&0x60 && mouEvent.fs&0x06))
    buttons |= BUTTON3;
  else
    buttons &= ~BUTTON3;
  if(mouEvent.fs&0x06)
    buttons |= BUTTON1;
  else
    buttons &= ~BUTTON1;
  if(mouEvent.fs&0x01)
    buttons = 0;

  *b = buttons;
  draw_mouse(ptrLoc.col, ptrLoc.row, 1);
}

static void vio_uninitialise()
{
  MouSetDevStatus(&oldDeviceStatus, hMou);
  MouClose(hMou);
  VioSetMode(&orig_mode, (HVIO) 0);
}

static void vio_setpalette(ui_palette pal, int start, int end) {
    int i;
    _portaccess(0x03c8, 0x03c9);
    for(i=0;i<=end-start;i++) {
        _outp8(0x03c8, i);
  	_outp8(0x03c9, pal[i][0]/4);
  	_outp8(0x03c9, pal[i][1]/4);
  	_outp8(0x03c9, pal[i][2]/4);
    }
    _outp8(0x03c8, 253);
    _outp8(0x03c9, 0);
    _outp8(0x03c9, 0);
    _outp8(0x03c9, 0);
    _outp8(0x03c8, 254);
    _outp8(0x03c9, 0);
    _outp8(0x03c9, 0);
    _outp8(0x03c9, 40);
    _outp8(0x03c8, 255);
    _outp8(0x03c9, 255);
    _outp8(0x03c9, 255);
    _outp8(0x03c9, 255);
}


static void vio_print(int x, int y, CONST char *text)
{
  char c, t, status;
  int i, idx, cx, cy;
  fprintf(stderr, "x=%d, y=%d\n", x, y);

  if (y > height - os2vio_driver.textheight)
    return;

  VioScrLock(1, &status, (HVIO) 0);
  while((c=*text++)!='\0') {
    cy = y;
    if (cy >= 200) {
	VioScrUnLock((HVIO)0);
	return;
      }
    for (i=0; i < os2vio_driver.textheight; i++) {
      cx = x;
      if (cx >= 320) {
	VioScrUnLock((HVIO)0);
	return;
      }
      t = font8[c*8+i];
      for (idx = 0; idx < 8; idx++) {
	if (t & 0x80)
	  ScrPtr[cy*width+cx++] = 255;
	else
	  ScrPtr[cy*width+cx++] = 254;
	t = t << 1;
      }
      cy++;
    }
    x+=os2vio_driver.textwidth;
  }	
  VioScrUnLock((HVIO)0);
}

static void vio_display()
{
  PTRLOC ptrLoc;
  char status;

  /* lock screen, if possible */
  VioScrLock(1, &status, (HVIO) 0);
  MouGetPtrPos(&ptrLoc, hMou);
  memcpy(ScrPtr, buffers[currentbuff], page_size);

  draw_mouse(ptrLoc.col, ptrLoc.row, 0);
  VioScrUnLock((HVIO)0);
}

static int vio_alloc(char **b1, char **b2)
{
  *b1 = buffers[0] = (char *)malloc(width*height);
  *b2 = buffers[1] = (char *)malloc(width*height);
  currentbuff = 0;

  return(width);
}

void vio_free(char *b1, char *b2) 
{
  free(buffers[0]);
  free(buffers[1]);
}

static void vio_flip_buffers() 
{
  currentbuff ^= 1;
}

static void vio_clear(void)
{
  memset(ScrPtr, 0, page_size);
}

static char *helptext[] =
{
  "OS/2 VIO DRIVER VERSION 1.1            ", 
  "=========================              ", 
  " I really like this driver, because    ",
  " I much prefer full screen zooming     ",
  " instead of small 320x200 window on    ",
  " the OS/2 Desktop.                     ",
  " This XaoS driver is for OS/2 VIO mode ",
  " and is fully featured.                ",
  " The following problems can ocour:     ",
  " o It doesn't start                    ",
  "    Thats can not be true. You can see ",
  "    this text!                         ",
  "                                       ",
  " As can be seen, I managed to get the  ",
  " MouAPI working in VIO fullscreen...   ",
  " What a fight is was..                 ",
  "                                       ", 
  "      OS/2 VIO driver was done by      ", 
  "      Thomas A. K. Kjaer  (C) 1996/97  ", 
  "      takjaer@imv.aau.dk               "
};

#define UGLYTEXTSIZE (sizeof(helptext)/sizeof(char *))
static struct params params[] =
{
  {NULL, 0, NULL, NULL}
};

struct ui_driver os2vio_driver = 
{
  "OS2VIO", 
  vio_init, 
  vio_get_size, 
  vio_processevents, 
  vio_getmouse, 
  vio_uninitialise, 
  NULL,
  vio_setpalette,
  vio_print, 
  vio_display, 
  vio_alloc, 
  vio_free, 
  vio_flip_buffers, 
  vio_clear,
  vio_mousetype,
  NULL,
  NULL,
  8,
  8,
  helptext, 
  UGLYTEXTSIZE, 
  params,
  FULLSCREEN | UPDATE_AFTER_RESIZE | PALETTE_ROTATION | ROTATE_INSIDE_CALCULATION,
  0.0, 0.0,
  0, 0,
  UI_C256,
  0, 252, 253
};

#endif
