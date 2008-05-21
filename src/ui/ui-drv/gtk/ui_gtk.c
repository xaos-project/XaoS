#include "aconfig.h"
#ifdef GTK_DRIVER
/*includes */
#include <cairo.h>
#include <gtk/gtk.h>
#include <ui.h>

int xaos_gtk_width = 640;
int xaos_gtk_height = 480;

int xaos_gtk_current_surface;
cairo_surface_t *xaos_gtk_surface[2];

static gboolean
xaos_gtk_on_expose_event(GtkWidget *widget,
    GdkEventExpose *event,
    gpointer data)
{
  cairo_t *cr;

  cr = gdk_cairo_create (widget->window);

  cairo_set_source_surface (cr, xaos_gtk_surface[xaos_gtk_current_surface], 0, 0);
  cairo_paint (cr);

  cairo_destroy (cr);

  return FALSE;
}

xaos_gtk_build_menu (struct uih_context *uih, CONST char *name)
{
}

static void
xaos_gtk_print (int x, int y, CONST char *text)
{
}

static void
xaos_gtk_display ()
{
}

static void
xaos_gtk_flip_buffers ()
{
  xaos_gtk_current_surface ^= 1;
}

void
xaos_gtk_free_buffers (char *b1, char *b2)
{
  cairo_surface_destroy (xaos_gtk_surface[0]);
  cairo_surface_destroy (xaos_gtk_surface[1]);
}

int
xaos_gtk_alloc_buffers (char **b1, char **b2)
{
  xaos_gtk_surface[0] = cairo_image_surface_create (CAIRO_FORMAT_RGB24, 
		  xaos_gtk_width, xaos_gtk_height);
  xaos_gtk_surface[1] = cairo_image_surface_create (CAIRO_FORMAT_RGB24, 
		  xaos_gtk_width, xaos_gtk_height);

  *b1 = (char *)cairo_image_surface_get_data (xaos_gtk_surface[0]);
  *b2 = (char *)cairo_image_surface_get_data (xaos_gtk_surface[1]);

  xaos_gtk_current_surface = 0;

  return cairo_image_surface_get_stride (xaos_gtk_surface[0]);
}

static void
xaos_gtk_getsize (int *w, int *h)
{
  *w = xaos_gtk_width;
  *h = xaos_gtk_height;
}

static void
xaos_gtk_processevents (int wait, int *mx, int *my, int *mb, int *k)
{
  gtk_main_iteration();
  *mx = 0;
  *my = 0;
  *mb = 0;
  *k = 0;
}

static int
xaos_gtk_init ()
{
  GtkWidget *window;
  GtkWidget *darea;

  int argc = 0;
  char **argv = NULL;

  gtk_init (&argc, &argv);

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  darea = gtk_drawing_area_new();
  gtk_container_add(GTK_CONTAINER(window), darea);

  g_signal_connect(darea, "expose-event",
      G_CALLBACK(xaos_gtk_on_expose_event), NULL);

  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  gtk_window_set_default_size(GTK_WINDOW(window), 640, 480); 

  gtk_widget_show_all(window);

  return ( /*1 for sucess 0 for fail */ 1);
}

static void
xaos_gtk_uninit ()
{
}

static void
xaos_gtk_getmouse (int *x, int *y, int *b)
{
  *x = 0;
  *y = 0;
  *b = 0;
}


static void
xaos_gtk_mousetype (int type)
{
}

static struct params xaos_gtk_params[] = {
  {"", P_HELP, NULL, "GTK+ driver options:"},
  {NULL, 0, NULL, NULL}
};

struct gui_driver gtk_gui_driver = {
    /* dorootmenu */	xaos_gtk_build_menu,
    /* enabledisable */	NULL,
    /* popup */		NULL,
    /* dialog */	NULL,
    /* help */		NULL
};

struct ui_driver gtk_driver = {
    /* name */          "GTK+ Driver",
    /* init */          xaos_gtk_init,
    /* getsize */       xaos_gtk_getsize,
    /* processevents */ xaos_gtk_processevents,
    /* getmouse */      xaos_gtk_getmouse,
    /* uninit */        xaos_gtk_uninit,
    /* set_color */     NULL,
    /* set_range */     NULL,
    /* print */         xaos_gtk_print,
    /* display */       xaos_gtk_display,
    /* alloc_buffers */ xaos_gtk_alloc_buffers,
    /* free_buffers */  xaos_gtk_free_buffers,
    /* filp_buffers */  xaos_gtk_flip_buffers,
    /* mousetype */     xaos_gtk_mousetype,
    /* flush */         NULL,
    /* textwidth */     12,
    /* textheight */    12,
    /* params */        xaos_gtk_params,
    /* flags */         RESOLUTION | PIXELSIZE | NOFLUSHDISPLAY | FULLSCREEN | PALETTE_ROTATION | ROTATE_INSIDE_CALCULATION,
    /* width */         0.01, 
    /* height */        0.01,
    /* maxwidth */      0, 
    /* maxheight */     0,
    /* imagetype */     UI_TRUECOLOR,
    /* palettestart */  0, 
    /* paletteend */    0, 
    /* maxentries */    0,
    /* rmask */         0x00ff0000,
    /* gmask */         0x0000ff00,
    /* bmask */         0x000000ff,
    /* gui_driver */    NULL
};

/* DONT FORGET TO ADD DOCUMENTATION ABOUT YOUR DRIVER INTO xaos.hlp FILE!*/

#endif
