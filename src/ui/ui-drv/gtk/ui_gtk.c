#include "aconfig.h"
#ifdef GTK_DRIVER
/*includes */
#include <cairo.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <ui.h>

int width = 640;
int height = 480;

int mouse_x = 0;
int mouse_y = 0;
int mouse_buttons = 0;
int keys = 0;

GtkWidget *drawing_area;
GtkWidget *menu_bar;
GtkWidget *window;
GHashTable *menuitem_table = NULL;

int current_surface;
cairo_surface_t *surface[2];

static void window_destroyed(GtkWidget * widget, gpointer data)
{
    ui_quit();
}

static gboolean
mouse_moved(GtkWidget * widget,
					 GdkEventMotion * event,
					 gpointer data)
{
    mouse_x = event->x;
    mouse_y = event->y;
    return TRUE;
}

static gboolean
key_pressed(GtkWidget * widget,
				     GdkEventKey * event, gpointer data)
{
    guint32 key;

    switch (event->keyval) {
    case GDK_Left:
	keys |= 1;
	ui_key(UIKEY_LEFT);
	break;
    case GDK_Right:
	keys |= 2;
	ui_key(UIKEY_RIGHT);
	break;
    case GDK_Up:
	keys |= 4;
	ui_key(UIKEY_UP);
	break;
    case GDK_Down:
	keys |= 8;
	ui_key(UIKEY_DOWN);
	break;
    case GDK_Page_Up:
	keys |= 4;
	ui_key(UIKEY_PGUP);
	break;
    case GDK_Page_Down:
	keys |= 8;
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
	key = gdk_keyval_to_unicode(event->keyval);
	ui_key(key);
    }
    return TRUE;
}

static gboolean
key_released(GtkWidget * widget,
				       GdkEventKey * event, gpointer data)
{
    switch (event->keyval) {
    case GDK_Left:
	keys &= ~1;
	break;
    case GDK_Right:
	keys &= ~2;
	break;
    case GDK_Up:
	keys &= ~4;
	break;
    case GDK_Down:
	keys &= ~8;
	break;
    case GDK_Page_Up:
	keys &= ~4;
	break;
    case GDK_Page_Down:
	keys &= ~8;
	break;
    }
    return TRUE;
}

static gboolean
button_pressed(GtkWidget * widget,
					GdkEventButton * event,
					gpointer data)
{
    switch (event->button) {
    case 1:
	mouse_buttons |= BUTTON1;
	break;
    case 2:
	mouse_buttons |= BUTTON2;
	break;
    case 3:
	mouse_buttons |= BUTTON3;
	break;
    }
    return TRUE;
}

static gboolean
button_released(GtkWidget * widget,
					  GdkEventButton * event,
					  gpointer data)
{
    switch (event->button) {
    case 1:
	mouse_buttons &= ~BUTTON1;
	break;
    case 2:
	mouse_buttons &= ~BUTTON2;
	break;
    case 3:
	mouse_buttons &= ~BUTTON3;
	break;
    }
    return TRUE;
}

static gboolean
drawing_area_resized (GtkWidget * widget, GdkEventConfigure * event)
{
    if (surface[0] && surface[1]) {
	width = event->width;
	height = event->height;
	ui_resize();
    }
}

 static gboolean
drawing_area_exposed(GtkWidget * widget,
				  GdkEventExpose * event, gpointer data)
{
    cairo_t *cr;

    cr = gdk_cairo_create(widget->window);

    cairo_set_source_surface(cr, surface[current_surface], 0, 0);
    cairo_paint(cr);

    cairo_destroy(cr);

    return FALSE;
}

void menuitem_activated(GtkMenuItem * item, gpointer data)
{
    /* 
     * gtk emits activate signal when radio buttons are deactivated as well.
     * we want to ignore these signals.
     */

    if (!GTK_IS_RADIO_MENU_ITEM(item)
	|| gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(item)))
	ui_menuactivate((CONST menuitem *) data, NULL);
}

static void
build_menu(struct uih_context *uih, CONST char *name,
		GtkWidget * parent)
{
    CONST menuitem *item;
    gchar *menulabel;
    GtkWidget *menuitem;
    GtkWidget *submenu;
    GSList *group = NULL;

    int i;

    for (i = 0; (item = menu_item(name, i)) != NULL; i++) {
	if (item->type == MENU_SEPARATOR)
	    menuitem = gtk_separator_menu_item_new();
	else {
	    if (item->type == MENU_CUSTOMDIALOG
		|| item->type == MENU_DIALOG)
		menulabel = g_strconcat(item->name, "...", NULL);
	    else
		menulabel = g_strdup(item->name);

	    if (item->flags & MENUFLAG_CHECKBOX)
		menuitem = gtk_check_menu_item_new_with_label(menulabel);
	    else if (item->flags & MENUFLAG_RADIO) {
		menuitem =
		    gtk_radio_menu_item_new_with_label(group, menulabel);
		group =
		    gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM
						  (menuitem));
	    } else
		menuitem = gtk_menu_item_new_with_label(menulabel);

	    if (menu_enabled(item, uih))
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM
					       (menuitem), TRUE);

	    g_free(menulabel);

	    g_hash_table_insert(menuitem_table, item->shortname,
				menuitem);
	}

	gtk_menu_shell_append(GTK_MENU_SHELL(parent), menuitem);
	gtk_widget_show(menuitem);

	if (item->type == MENU_SUBMENU) {
	    submenu = gtk_menu_new();
	    build_menu(uih, item->shortname, submenu);
	    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), submenu);
	} else {
	    g_signal_connect(G_OBJECT(menuitem), "activate",
			     G_CALLBACK(menuitem_activated),
			     (gpointer) item);
	}
    }
}

static void build_root_menu(struct uih_context *uih, CONST char *name)
{
    if (menuitem_table)
	g_hash_table_destroy(menuitem_table);

    gtk_container_foreach(GTK_CONTAINER(menu_bar),
			  (GtkCallback)gtk_widget_destroy, NULL);

    menuitem_table = g_hash_table_new(g_str_hash, g_str_equal);
    build_menu(uih, name, menu_bar);
}

static void toggle_menuitem(struct uih_context *uih, CONST char *name)
{
    if (menuitem_table) {
    	GtkMenuItem *menuitem = g_hash_table_lookup(menuitem_table, name);

	g_signal_handlers_block_matched(menuitem, G_SIGNAL_MATCH_FUNC, 
					0, 0, 0, menuitem_activated, 0);

        gtk_check_menu_item_set_active(menuitem, 
				!gtk_check_menu_item_get_active(menuitem));

	g_signal_handlers_unblock_matched(menuitem, G_SIGNAL_MATCH_FUNC, 
					  0, 0, 0, menuitem_activated, 0);
    }
}

static void print(int x, int y, CONST char *text)
{
}

static void display()
{
    gtk_widget_queue_draw(drawing_area);
}

static void flip_buffers()
{
    current_surface ^= 1;
}

void free_buffers(char *b1, char *b2)
{
    cairo_surface_destroy(surface[0]);
    cairo_surface_destroy(surface[1]);
    surface[0] = NULL;
    surface[1] = NULL;
}

int alloc_buffers(char **b1, char **b2)
{
    surface[0] = cairo_image_surface_create(CAIRO_FORMAT_RGB24,
						 width, height);
    surface[1] = cairo_image_surface_create(CAIRO_FORMAT_RGB24,
						 width, height);

    *b1 = (char *) cairo_image_surface_get_data(surface[0]);
    *b2 = (char *) cairo_image_surface_get_data(surface[1]);

    current_surface = 0;

    return cairo_image_surface_get_stride(surface[0]);
}

static void getsize(int *w, int *h)
{
    *w = width;
    *h = height;
}

static void processevents(int wait, int *mx, int *my, int *mb, int *k)
{
    while (gtk_events_pending())
	gtk_main_iteration_do(wait ? TRUE : FALSE);

    *mx = mouse_x;
    *my = mouse_y;
    *mb = mouse_buttons;
    *k = keys;
}

static int init()
{
    GtkWidget *vbox;

    int argc = 0;
    char **argv = NULL;

    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(window), 640, 480);
    gtk_window_set_title(GTK_WINDOW(window), "XaoS");

    g_signal_connect(G_OBJECT(window), "destroy",
		     G_CALLBACK(window_destroyed), NULL);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    gtk_widget_show(vbox);

    menu_bar = gtk_menu_bar_new();
    gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, FALSE, 0);
    gtk_widget_show(menu_bar);

    drawing_area = gtk_drawing_area_new();
    gtk_box_pack_end(GTK_BOX(vbox), drawing_area, TRUE, TRUE, 0);
    gtk_widget_show(drawing_area);

    GTK_WIDGET_SET_FLAGS(drawing_area, GTK_CAN_FOCUS);
    gtk_widget_grab_focus(drawing_area);

    gtk_widget_add_events(drawing_area,
			  GDK_POINTER_MOTION_MASK |
			  GDK_BUTTON_PRESS_MASK |
			  GDK_BUTTON_RELEASE_MASK |
			  GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);

    g_signal_connect(G_OBJECT(drawing_area), "motion-notify-event",
		     G_CALLBACK(mouse_moved),
		     NULL);

    g_signal_connect(G_OBJECT(drawing_area), "button-press-event",
		     G_CALLBACK(button_pressed),
		     NULL);

    g_signal_connect(G_OBJECT(drawing_area), "button-release-event",
		     G_CALLBACK(button_released),
		     NULL);

    g_signal_connect(G_OBJECT(drawing_area), "key-press-event",
		     G_CALLBACK(key_pressed),
		     NULL);

    g_signal_connect(G_OBJECT(drawing_area), "key-release-event",
		     G_CALLBACK(key_released),
		     NULL);

    g_signal_connect(G_OBJECT(drawing_area), "expose-event",
		     G_CALLBACK(drawing_area_exposed), NULL);

    g_signal_connect(G_OBJECT(drawing_area), "configure-event",
		     G_CALLBACK(drawing_area_resized), NULL);

    gtk_widget_show_all(window);

    return ( /*1 for sucess 0 for fail */ 1);
}

static void uninit()
{
    g_signal_handlers_disconnect_matched(window, G_SIGNAL_MATCH_FUNC, 
					 0, 0, 0, window_destroyed, 0);

    gtk_widget_destroy(window);
}

static void getmouse(int *x, int *y, int *b)
{
    *x = mouse_x;
    *y = mouse_y;
    *b = mouse_buttons;
}


static void mousetype(int type)
{
}

static struct params params[] = {
    {"", P_HELP, NULL, "GTK+ driver options:"},
    {NULL, 0, NULL, NULL}
};

struct gui_driver gtk_gui_driver = {
    /* dorootmenu */ build_root_menu,
    /* enabledisable */ toggle_menuitem,
    /* popup */ NULL,
    /* dialog */ NULL,
    /* help */ NULL
};

struct ui_driver gtk_driver = {
    /* name */ "GTK+ Driver",
    /* init */ init,
    /* getsize */ getsize,
    /* processevents */ processevents,
    /* getmouse */ getmouse,
    /* uninit */ uninit,
    /* set_color */ NULL,
    /* set_range */ NULL,
    /* print */ print,
    /* display */ display,
    /* alloc_buffers */ alloc_buffers,
    /* free_buffers */ free_buffers,
    /* filp_buffers */ flip_buffers,
    /* mousetype */ mousetype,
    /* flush */ NULL,
    /* textwidth */ 12,
    /* textheight */ 12,
    /* params */ params,
    /* flags */
	RESOLUTION | PIXELSIZE | NOFLUSHDISPLAY | FULLSCREEN |
	PALETTE_ROTATION | ROTATE_INSIDE_CALCULATION,
    /* width */ 0.01,
    /* height */ 0.01,
    /* maxwidth */ 0,
    /* maxheight */ 0,
    /* imagetype */ UI_TRUECOLOR,
    /* palettestart */ 0,
    /* paletteend */ 0,
    /* maxentries */ 0,
    /* rmask */ 0x00ff0000,
    /* gmask */ 0x0000ff00,
    /* bmask */ 0x000000ff,
    /* gui_driver */ &gtk_gui_driver
};

/* DONT FORGET TO ADD DOCUMENTATION ABOUT YOUR DRIVER INTO xaos.hlp FILE!*/

#endif
