#include "aconfig.h"
#ifdef GTK_DRIVER
/*includes */
#include <cairo.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <ui.h>

int xgtk_width = 640;
int xgtk_height = 480;

int xgtk_mouse_x = 0;
int xgtk_mouse_y = 0;
int xgtk_mouse_buttons = 0;
int xgtk_keys = 0;

GtkWidget *xgtk_drawing_area;
GtkWidget *xgtk_menu_bar;
GHashTable *xgtk_menuitem_table = NULL;

int xgtk_current_surface;
cairo_surface_t *xgtk_surface[2];

static void 
xgtk_on_destroy( GtkWidget *widget, 
    gpointer   data )
{
  ui_quit();
}

static gboolean
xgtk_drawing_area_on_motion_notify_event (GtkWidget *widget,
    GdkEventMotion *event,
    gpointer data)
{
  xgtk_mouse_x = event->x;
  xgtk_mouse_y = event->y;
  return TRUE;
}

static gboolean
xgtk_drawing_area_on_key_press_event (GtkWidget *widget,
    GdkEventKey *event,
    gpointer data)
{
  guint32 key;

  switch (event->keyval) {
    case GDK_Left:
      xgtk_keys |= 1;
      ui_key(UIKEY_LEFT);
      break;
    case GDK_Right:
      xgtk_keys |= 2;
      ui_key(UIKEY_RIGHT);
      break;
    case GDK_Up:
      xgtk_keys |= 4;
      ui_key(UIKEY_UP);
      break;
    case GDK_Down:
      xgtk_keys |= 8;
      ui_key(UIKEY_DOWN);
      break;
    case GDK_Page_Up:
      xgtk_keys |= 4;
      ui_key(UIKEY_PGUP);
      break;
    case GDK_Page_Down:
      xgtk_keys |= 8;
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
xgtk_drawing_area_on_key_release_event (GtkWidget *widget,
    GdkEventKey *event,
    gpointer data)
{
  switch (event->keyval) {
    case GDK_Left:
      xgtk_keys &= ~1;
      break;
    case GDK_Right:
      xgtk_keys &= ~2;
      break;
    case GDK_Up:
      xgtk_keys &= ~4;
      break;
    case GDK_Down:
      xgtk_keys &= ~8;
      break;
    case GDK_Page_Up:
      xgtk_keys &= ~4;
      break;
    case GDK_Page_Down:
      xgtk_keys &= ~8;
      break;
  }
  return TRUE;
}

static gboolean
xgtk_drawing_area_on_button_press_event (GtkWidget *widget,
    GdkEventButton *event,
    gpointer data)
{
  switch (event->button) {
    case 1:
      xgtk_mouse_buttons |= BUTTON1;
      break;
    case 2:
      xgtk_mouse_buttons |= BUTTON2;
      break;
    case 3:
      xgtk_mouse_buttons |= BUTTON3;
      break;
  }
  return TRUE;
}

static gboolean
xgtk_drawing_area_on_button_release_event (GtkWidget *widget,
    GdkEventButton *event,
    gpointer data)
{
  switch (event->button) {
    case 1:
      xgtk_mouse_buttons &= ~BUTTON1;
      break;
    case 2:
      xgtk_mouse_buttons &= ~BUTTON2;
      break;
    case 3:
      xgtk_mouse_buttons &= ~BUTTON3;
      break;
  }
  return TRUE;
}

static gboolean
xgtk_drawing_area_on_expose_event (GtkWidget *widget,
    GdkEventExpose *event,
    gpointer data)
{
  cairo_t *cr;

  cr = gdk_cairo_create (widget->window);

  cairo_set_source_surface (cr, xgtk_surface[xgtk_current_surface], 0, 0);
  cairo_paint (cr);

  cairo_destroy (cr);

  return FALSE;
}

void
xgtk_menuitem_on_activate (GtkMenuItem *item,
    gpointer data)
{
  /* 
   * gtk emits activate signal when radio buttons are deactivated as well.
   * we want to ignore these signals.
   */

  if (!GTK_IS_RADIO_MENU_ITEM(item) || gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(item)))
     ui_menuactivate((CONST menuitem *)data, NULL);
}

static void
xgtk_build_menu (struct uih_context *uih, CONST char *name, GtkWidget *parent)
{
  CONST menuitem *item;
  gchar *menulabel;
  GtkWidget *menuitem;
  GtkWidget *submenu;
  GSList *group = NULL;

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

	  if (item->flags & MENUFLAG_CHECKBOX)
	      menuitem = gtk_check_menu_item_new_with_label (menulabel);
	  else if (item->flags & MENUFLAG_RADIO)
	  {
	      menuitem = gtk_radio_menu_item_new_with_label(group, menulabel);
	      group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (menuitem));
	  }
	  else
	      menuitem = gtk_menu_item_new_with_label (menulabel);

	  if (menu_enabled(item, uih))
	  	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem), TRUE);

	  g_free(menulabel);

          g_hash_table_insert(xgtk_menuitem_table, item->shortname, menuitem);
          printf("Added menuitem %x to hashtable %x\n", menuitem, xgtk_menuitem_table);
	}

      gtk_menu_shell_append (GTK_MENU_SHELL (parent), menuitem);
      gtk_widget_show (menuitem);

      if (item->type == MENU_SUBMENU)
        {
	  submenu = gtk_menu_new();
	  xgtk_build_menu(uih, item->shortname, submenu);
	  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), submenu);
	}
      else
        {
	  g_signal_connect (G_OBJECT (menuitem), "activate",
                            G_CALLBACK (xgtk_menuitem_on_activate),
			    (gpointer) item);
	}
    }
}

static void
xgtk_build_root_menu (struct uih_context *uih, CONST char *name)
{
  xgtk_menuitem_table = g_hash_table_new (g_str_hash, g_str_equal);
  xgtk_build_menu (uih, name, xgtk_menu_bar);
}

static void
xgtk_toggle_menuitem(struct uih_context *uih, CONST char *name)
{
  GtkMenuItem *menuitem = g_hash_table_lookup (xgtk_menuitem_table, name);
  //gtk_check_menu_item_set_active(menuitem, !gtk_check_menu_item_get_active(menuitem));
}

static void
xgtk_print (int x, int y, CONST char *text)
{
}

static void
xgtk_display ()
{
  gtk_widget_queue_draw (xgtk_drawing_area);
}

static void
xgtk_flip_buffers ()
{
  xgtk_current_surface ^= 1;
}

void
xgtk_free_buffers (char *b1, char *b2)
{
  cairo_surface_destroy (xgtk_surface[0]);
  cairo_surface_destroy (xgtk_surface[1]);
}

int
xgtk_alloc_buffers (char **b1, char **b2)
{
  xgtk_surface[0] = cairo_image_surface_create (CAIRO_FORMAT_RGB24, 
		  xgtk_width, xgtk_height);
  xgtk_surface[1] = cairo_image_surface_create (CAIRO_FORMAT_RGB24, 
		  xgtk_width, xgtk_height);

  *b1 = (char *)cairo_image_surface_get_data (xgtk_surface[0]);
  *b2 = (char *)cairo_image_surface_get_data (xgtk_surface[1]);

  xgtk_current_surface = 0;

  return cairo_image_surface_get_stride (xgtk_surface[0]);
}

static void
xgtk_getsize (int *w, int *h)
{
  *w = xgtk_width;
  *h = xgtk_height;
}

static void
xgtk_processevents (int wait, int *mx, int *my, int *mb, int *k)
{
  while (gtk_events_pending ())
    gtk_main_iteration_do (wait ? TRUE : FALSE);

  *mx = xgtk_mouse_x;
  *my = xgtk_mouse_y;
  *mb = xgtk_mouse_buttons;
  *k = xgtk_keys;
}

static int
xgtk_init ()
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
      G_CALLBACK (xgtk_on_destroy), NULL);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  gtk_widget_show (vbox);

  xgtk_menu_bar = gtk_menu_bar_new ();
  gtk_box_pack_start (GTK_BOX (vbox), xgtk_menu_bar, FALSE, FALSE, 0);
  gtk_widget_show(xgtk_menu_bar);

  xgtk_drawing_area = gtk_drawing_area_new();
  gtk_box_pack_end (GTK_BOX (vbox), xgtk_drawing_area, TRUE, TRUE, 0);
  gtk_widget_show (xgtk_drawing_area);

  GTK_WIDGET_SET_FLAGS (xgtk_drawing_area, GTK_CAN_FOCUS);
  gtk_widget_grab_focus (xgtk_drawing_area);

  gtk_widget_add_events (xgtk_drawing_area, 
      GDK_POINTER_MOTION_MASK | 
      GDK_BUTTON_PRESS_MASK | 
      GDK_BUTTON_RELEASE_MASK |
      GDK_KEY_PRESS_MASK | 
      GDK_KEY_RELEASE_MASK);

  g_signal_connect(G_OBJECT(xgtk_drawing_area), "motion-notify-event",
      G_CALLBACK(xgtk_drawing_area_on_motion_notify_event), NULL);

  g_signal_connect(G_OBJECT(xgtk_drawing_area), "button-press-event",
      G_CALLBACK(xgtk_drawing_area_on_button_press_event), NULL);

  g_signal_connect(G_OBJECT(xgtk_drawing_area), "button-release-event",
      G_CALLBACK(xgtk_drawing_area_on_button_release_event), NULL);

  g_signal_connect(G_OBJECT(xgtk_drawing_area), "key-press-event",
      G_CALLBACK(xgtk_drawing_area_on_key_press_event), NULL);

  g_signal_connect(G_OBJECT(xgtk_drawing_area), "key-release-event",
      G_CALLBACK(xgtk_drawing_area_on_key_release_event), NULL);

  g_signal_connect(G_OBJECT(xgtk_drawing_area), "expose-event",
      G_CALLBACK(xgtk_drawing_area_on_expose_event), NULL);

  gtk_widget_show_all(window);

  return ( /*1 for sucess 0 for fail */ 1);
}

static void
xgtk_uninit ()
{
}

static void
xgtk_getmouse (int *x, int *y, int *b)
{
  *x = xgtk_mouse_x;
  *y = xgtk_mouse_y;
  *b = xgtk_mouse_buttons;
}


static void
xgtk_mousetype (int type)
{
}

static struct params xgtk_params[] = {
  {"", P_HELP, NULL, "GTK+ driver options:"},
  {NULL, 0, NULL, NULL}
};

struct gui_driver gtk_gui_driver = {
    /* dorootmenu */	xgtk_build_root_menu,
    /* enabledisable */	xgtk_toggle_menuitem,
    /* popup */		NULL,
    /* dialog */	NULL,
    /* help */		NULL
};

struct ui_driver gtk_driver = {
    /* name */          "GTK+ Driver",
    /* init */          xgtk_init,
    /* getsize */       xgtk_getsize,
    /* processevents */ xgtk_processevents,
    /* getmouse */      xgtk_getmouse,
    /* uninit */        xgtk_uninit,
    /* set_color */     NULL,
    /* set_range */     NULL,
    /* print */         xgtk_print,
    /* display */       xgtk_display,
    /* alloc_buffers */ xgtk_alloc_buffers,
    /* free_buffers */  xgtk_free_buffers,
    /* filp_buffers */  xgtk_flip_buffers,
    /* mousetype */     xgtk_mousetype,
    /* flush */         NULL,
    /* textwidth */     12,
    /* textheight */    12,
    /* params */        xgtk_params,
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
