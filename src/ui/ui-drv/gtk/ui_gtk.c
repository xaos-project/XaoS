#include "aconfig.h"
#ifdef GTK_DRIVER
/*includes */
#include <cairo.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <ui.h>

int xaos_gtk_width = 640;
int xaos_gtk_height = 480;

int xaos_gtk_mouse_x = 0;
int xaos_gtk_mouse_y = 0;
int xaos_gtk_mouse_buttons = 0;
int xaos_gtk_keys = 0;

GtkWidget *xaos_gtk_drawing_area;

GtkWidget *xaos_gtk_menu_bar;

int xaos_gtk_current_surface;
cairo_surface_t *xaos_gtk_surface[2];

static void 
xaos_gtk_on_destroy( GtkWidget *widget, 
    gpointer   data )
{
  ui_quit();
}

static gboolean
xaos_gtk_on_motion_notify_event (GtkWidget *widget,
    GdkEventMotion *event,
    gpointer data)
{
  xaos_gtk_mouse_x = event->x;
  xaos_gtk_mouse_y = event->y;
  return TRUE;
}

static gboolean
xaos_gtk_on_key_press_event (GtkWidget *widget,
    GdkEventKey *event,
    gpointer data)
{
  guint32 key;

  switch (event->keyval) {
    case GDK_Left:
      xaos_gtk_keys |= 1;
      ui_key(UIKEY_LEFT);
      break;
    case GDK_Right:
      xaos_gtk_keys |= 2;
      ui_key(UIKEY_RIGHT);
      break;
    case GDK_Up:
      xaos_gtk_keys |= 4;
      ui_key(UIKEY_UP);
      break;
    case GDK_Down:
      xaos_gtk_keys |= 8;
      ui_key(UIKEY_DOWN);
      break;
    case GDK_Page_Up:
      xaos_gtk_keys |= 4;
      ui_key(UIKEY_PGUP);
      break;
    case GDK_Page_Down:
      xaos_gtk_keys |= 8;
      ui_key(UIKEY_PGDOWN);
      break;
    case GDK_BackSpace:
      ui_key(UIKEY_BACKSPACE);
      break;
    case GDK_Escape:
      ui_key(UIKEY_ESC);
      break;
    case GDK_Home:
      ui_key(UIKEY_HOME);
      break;
    case GDK_End:
      ui_key(UIKEY_END);
      break;
    case GDK_Tab:
      ui_key(UIKEY_TAB);
      break;
    default:
      key = gdk_keyval_to_unicode (event->keyval);
      ui_key(key);
  }
  return TRUE;
}

static gboolean
xaos_gtk_on_key_release_event (GtkWidget *widget,
    GdkEventKey *event,
    gpointer data)
{
  switch (event->keyval) {
    case GDK_Left:
      xaos_gtk_keys &= ~1;
      break;
    case GDK_Right:
      xaos_gtk_keys &= ~2;
      break;
    case GDK_Up:
      xaos_gtk_keys &= ~4;
      break;
    case GDK_Down:
      xaos_gtk_keys &= ~8;
      break;
    case GDK_Page_Up:
      xaos_gtk_keys &= ~4;
      break;
    case GDK_Page_Down:
      xaos_gtk_keys &= ~8;
      break;
  }
  return TRUE;
}

static gboolean
xaos_gtk_on_button_press_event (GtkWidget *widget,
    GdkEventButton *event,
    gpointer data)
{
  switch (event->button) {
    case 1:
      xaos_gtk_mouse_buttons |= BUTTON1;
      break;
    case 2:
      xaos_gtk_mouse_buttons |= BUTTON2;
      break;
    case 3:
      xaos_gtk_mouse_buttons |= BUTTON3;
      break;
  }
  return TRUE;
}

static gboolean
xaos_gtk_on_button_release_event (GtkWidget *widget,
    GdkEventButton *event,
    gpointer data)
{
  switch (event->button) {
    case 1:
      xaos_gtk_mouse_buttons &= ~BUTTON1;
      break;
    case 2:
      xaos_gtk_mouse_buttons &= ~BUTTON2;
      break;
    case 3:
      xaos_gtk_mouse_buttons &= ~BUTTON3;
      break;
  }
  return TRUE;
}

static gboolean
xaos_gtk_on_expose_event (GtkWidget *widget,
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

void
xaos_gtk_menuitem_on_activate (GtkMenuItem *item, gpointer data)
{
  ui_menuactivate((CONST menuitem *)data, NULL);
}

static void
xaos_gtk_add_menu (struct uih_context *uih, CONST char *name, GtkWidget *parent)
{
  CONST menuitem *item;
  gchar *menulabel;
  GtkWidget *menuitem;
  GtkWidget *submenu;
  int i;

  for (i = 0; (item = menu_item(name, i)) != NULL; i++)
    {
      if (item->type == MENU_SEPARATOR)
	  menuitem = gtk_separator_menu_item_new ();
      else
        {
	  if (item->type == MENU_CUSTOMDIALOG || item->type == MENU_DIALOG)
              menulabel = g_strconcat (item->name, "...", NULL);
	  else
	      menulabel = g_strdup (item->name);

	  if (item->flags & (MENUFLAG_RADIO | MENUFLAG_CHECKBOX))
	    {
	      menuitem = gtk_check_menu_item_new_with_label (menulabel);
	      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem),
			                      menu_enabled(item, uih));
	      gtk_check_menu_item_set_draw_as_radio (GTK_CHECK_MENU_ITEM (menuitem),
			                             item->flags & MENUFLAG_RADIO);
	    }
	  else
	      menuitem = gtk_menu_item_new_with_label (menulabel);

	  g_free(menulabel);
	}

      gtk_menu_shell_append (GTK_MENU_SHELL (parent), menuitem);
      gtk_widget_show (menuitem);

      if (item->type == MENU_SUBMENU)
        {
	  submenu = gtk_menu_new();
	  xaos_gtk_add_menu(uih, item->shortname, submenu);
	  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), submenu);
	}
      else
        {
	  g_signal_connect (G_OBJECT (menuitem), "activate",
                            G_CALLBACK (xaos_gtk_menuitem_on_activate),
			    (gpointer) item);
	      
	}
    }
}

static void
xaos_gtk_build_menu (struct uih_context *uih, CONST char *name)
{
  xaos_gtk_add_menu (uih, name, xaos_gtk_menu_bar);
}

static void
xaos_gtk_print (int x, int y, CONST char *text)
{
}

static void
xaos_gtk_display ()
{
  gtk_widget_queue_draw (xaos_gtk_drawing_area);
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
  gtk_main_iteration_do(wait? TRUE : FALSE);
  *mx = xaos_gtk_mouse_x;
  *my = xaos_gtk_mouse_y;
  *mb = xaos_gtk_mouse_buttons;
  *k = xaos_gtk_keys;
}

static int
xaos_gtk_init ()
{
  GtkWidget *window;
  GtkWidget *vbox;

  int argc = 0;
  char **argv = NULL;

  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
  gtk_window_set_default_size (GTK_WINDOW (window), 640, 480); 
  gtk_window_set_title (GTK_WINDOW (window), "XaoS");

  g_signal_connect (G_OBJECT(window), "destroy", 
      G_CALLBACK (xaos_gtk_on_destroy), NULL);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  gtk_widget_show (vbox);

  xaos_gtk_menu_bar = gtk_menu_bar_new ();
  gtk_box_pack_start (GTK_BOX (vbox), xaos_gtk_menu_bar, FALSE, FALSE, 0);
  gtk_widget_show(xaos_gtk_menu_bar);

  xaos_gtk_drawing_area = gtk_drawing_area_new();
  gtk_box_pack_end (GTK_BOX (vbox), xaos_gtk_drawing_area, TRUE, TRUE, 0);
  gtk_widget_show (xaos_gtk_drawing_area);

  GTK_WIDGET_SET_FLAGS (xaos_gtk_drawing_area, GTK_CAN_FOCUS);
  gtk_widget_grab_focus (xaos_gtk_drawing_area);

  gtk_widget_add_events (xaos_gtk_drawing_area, 
      GDK_POINTER_MOTION_MASK | 
      GDK_BUTTON_PRESS_MASK | 
      GDK_BUTTON_RELEASE_MASK |
      GDK_KEY_PRESS_MASK | 
      GDK_KEY_RELEASE_MASK);

  g_signal_connect(G_OBJECT(xaos_gtk_drawing_area), "motion-notify-event",
      G_CALLBACK(xaos_gtk_on_motion_notify_event), NULL);

  g_signal_connect(G_OBJECT(xaos_gtk_drawing_area), "button-press-event",
      G_CALLBACK(xaos_gtk_on_button_press_event), NULL);

  g_signal_connect(G_OBJECT(xaos_gtk_drawing_area), "button-release-event",
      G_CALLBACK(xaos_gtk_on_button_release_event), NULL);

  g_signal_connect(G_OBJECT(xaos_gtk_drawing_area), "key-press-event",
      G_CALLBACK(xaos_gtk_on_key_press_event), NULL);

  g_signal_connect(G_OBJECT(xaos_gtk_drawing_area), "key-release-event",
      G_CALLBACK(xaos_gtk_on_key_release_event), NULL);

  g_signal_connect(G_OBJECT(xaos_gtk_drawing_area), "expose-event",
      G_CALLBACK(xaos_gtk_on_expose_event), NULL);

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
  *x = xaos_gtk_mouse_x;
  *y = xaos_gtk_mouse_y;
  *b = xaos_gtk_mouse_buttons;
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
    /* gui_driver */    &gtk_gui_driver
};

/* DONT FORGET TO ADD DOCUMENTATION ABOUT YOUR DRIVER INTO xaos.hlp FILE!*/

#endif