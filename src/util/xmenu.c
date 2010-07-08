#ifdef _plan9_
#include <u.h>
#include <libc.h>
#else
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#endif
#include <config.h>
#include <filter.h>
#include <fractal.h>
#include <ui_helper.h>
#include <xerror.h>
#include <xldio.h>
#include <misc-f.h>
#include "config.h"
#include "xmenu.h"

#define HASHBITS 8
#define HASHSIZE (1<<HASHBITS)
#define HASHMASK (HASHSIZE-1)
#define HASH(c,len) (((len)*32+(c)[0]+(c)[(len)-1])&HASHMASK)

static struct queuelist {
    struct queuelist *next;
    struct queuelist *previous;
    const menuitem *item;
    dialogparam *d;
} *firstqueue = NULL, *lastqueue = NULL;
static struct entry {
    struct entry *next;
    struct entry *previous;
    struct entry *nextname;
    const menuitem *item;
} *firstitem = NULL, *lastitem = NULL;

struct entry *namehash[HASHSIZE];

static void
x_menu_insert(const menuitem * item, struct entry *iitem, int n)
{
    int i;
    int len;
    int hashpos;
    struct entry *list;
    for (i = 0; i < n; i++) {
	list = (struct entry *) calloc(1, sizeof(struct queuelist));
	if (list == NULL) {
	    x_error("Warning:out of memory!");
	    return;
	}
	if (item->type != MENU_SEPARATOR) {
	    len = strlen(item->shortname);
	    hashpos = HASH(item->shortname, len);
	    list->nextname = namehash[hashpos];
#ifdef DEBUG
	    {
		struct entry *e = list->nextname;
		while (e != NULL) {
		    if (e->item->type != MENU_SUBMENU
			&& e->item->type != MENU_SEPARATOR
			&& item->type != MENU_SEPARATOR);
		    if (!strcmp(e->item->shortname, item->shortname)
			/*&& e->item->type != MENU_SUBMENU && item->type != MENU_SUBMENU */
			) {
			x_error
			    ("Menu error:Name collision %s:'%s'(%s) and '%s'(%s)",
			     item->shortname, item->name, item->menuname,
			     e->item->name, e->item->menuname);
		    }
		    e = e->nextname;
		}
	    }
#endif
	    namehash[hashpos] = list;
	}
	list->item = item;
	if (iitem == NULL) {
	    /*printf("ahoj\n"); */
	    list->previous = lastitem;
	    list->next = NULL;
	    if (lastitem != NULL)
		lastitem->next = list;
	    else
		firstitem = list;
	    lastitem = list;
	} else {
	    list->next = iitem;
	    list->previous = iitem->previous;
	    if (iitem->previous)
		iitem->previous->next = list;
	    else
		firstitem = list;
	    iitem->previous = list;
	}
	item++;
    }
}

void menu_add(const menuitem * item, int n)
{
    x_menu_insert(item, NULL, n);
}

void menu_insert(const menuitem * item, const char *before, int n)
{
    struct entry *e = firstitem;
    while (e != NULL) {
	if (!strcmp(e->item->shortname, before))
	    break;
	e = e->next;
    }
    x_menu_insert(item, e, n);
}

void menu_delete(const menuitem * items, int n)
{
    int d = 0, i;
    struct entry *item = firstitem;
    struct entry *pe;
    int hashpos;
    for (i = 0; i < n; i++) {
	if (items[i].type == MENU_SEPARATOR) {
	    struct entry *item = firstitem;
	    while (item && item->item != items + i)
		item = item->next;
	    if (!item)
		abort();
	    if (item->previous != NULL)
		item->previous->next = item->next;
	    else
		firstitem = item->next;
	    if (item->next != NULL)
		item->next->previous = item->previous;
	    else
		lastitem = item->previous;
	    free(item);
	} else {
	    int len = strlen(items[i].shortname);
	    hashpos = HASH(items[i].shortname, len);
	    pe = NULL;
	    item = namehash[hashpos];
	    while (item != NULL) {
		if (items + i == item->item) {
		    d++;
		    if (pe == NULL)
			namehash[hashpos] = item->nextname;
		    else
			pe->nextname = item->nextname;
		    if (item->previous != NULL)
			item->previous->next = item->next;
		    else
			firstitem = item->next;
		    if (item->next != NULL)
			item->next->previous = item->previous;
		    else
			lastitem = item->previous;
		    free(item);
		    break;
		}		/*if */
		pe = item;
		item = item->nextname;
	    }			/*while */
	}
#ifdef DEBUG
	if (item == NULL)
	    x_error("Item %s not found!", items[i].shortname);
#endif
    }				/*for */
}

void menu_addqueue(const menuitem * item, dialogparam * d)
{
    struct queuelist *list;
    list = (struct queuelist *) calloc(1, sizeof(struct queuelist));
    if (list == NULL) {
	x_error("Warning:out of memory!");
	return;
    }
    list->previous = lastqueue;
    list->next = NULL;
    list->item = item;
    list->d = d;
    if (lastqueue != NULL)
	lastqueue->next = list;
    else
	firstqueue = list;
    lastqueue = list;
}

const menuitem *menu_delqueue(dialogparam ** d)
{
    const struct menuitem *item;
    struct queuelist *list = firstqueue;
    if (firstqueue == NULL)
	return NULL;
    item = firstqueue->item;
    *d = firstqueue->d;
    firstqueue = list->next;
    if (list->next != NULL)
	list->next->previous = NULL;
    else
	lastqueue = NULL;
    free(list);
    return (item);
}

const static void *menu_rfind(const void
			      *(*function) (struct entry * item),
			      const char *root)
{
    struct entry *item = firstitem;
    const void *r;
    while (item != NULL) {
	if (!strcmp(root, item->item->menuname)) {
	    if ((r = function(item)) != NULL)
		return r;
	    if (item->item->type == MENU_SUBMENU
		&& (r =
		    menu_rfind(function, item->item->shortname)) != NULL)
		return r;
	}
	item = item->next;
    }
    return NULL;
}

const static char *findkey;
const static void *cmpfunction(struct entry *item)
{
    if (item->item->key == NULL)
	return NULL;
    if (!strcmp(findkey, item->item->key))
	return item->item;
    return NULL;
}

const menuitem *menu_findkey(const char *key, const char *root)
{
    findkey = key;
    return ((const menuitem *) menu_rfind(cmpfunction, root));
}

static const menuitem *finditem;
const static void *cmpfunction2(struct entry *item)
{
    if (item->item == finditem)
	return item;
    return NULL;
}

int menu_available(const menuitem * item, const char *root)
{
    finditem = item;
    return (menu_rfind(cmpfunction2, root) != NULL);
}

const char *menu_fullname(const char *menu)
{
    struct entry *item = firstitem;
    while (item != NULL) {
	if (item->item->type == MENU_SUBMENU
	    && !strcmp(menu, item->item->shortname)) {
	    return (item->item->name);
	}
	item = item->next;
    }
    return NULL;
}

const menuitem *menu_item(const char *menu, int n)
{
    struct entry *item = firstitem;
    while (item != NULL) {
	if (!strcmp(menu, item->item->menuname)) {
	    if (!(item->item->flags & MENUFLAG_NOMENU))
		n--;
	    if (n < 0)
		return (item->item);
	}
	item = item->next;
    }
    return NULL;
}

static const menuitem *menu_item2(const char *menu, int n)
{
    struct entry *item = firstitem;
    while (item != NULL) {
	if (!strcmp(menu, item->item->menuname)) {
	    n--;
	    if (n < 0)
		return (item->item);
	}
	item = item->next;
    }
    return NULL;
}

int menu_havedialog(const menuitem * item, struct uih_context *c)
{
    if (item->type != MENU_DIALOG && item->type != MENU_CUSTOMDIALOG)
	return 0;
    if (!(item->type & MENUFLAG_RADIO) || c == NULL)
	return 1;
    if (item->flags & MENUFLAG_DIALOGATDISABLE)
	return (menu_enabled(item, c));
    return (!menu_enabled(item, c));
}

static void menu_freeparam(dialogparam * d, const struct dialog *di)
{
    switch (di->type) {
    case DIALOG_STRING:
    case DIALOG_KEYSTRING:
    case DIALOG_IFILE:
    case DIALOG_OFILE:
	free(d->dstring);
    }
}

void
menu_destroydialog(const menuitem * item, dialogparam * d,
		   struct uih_context *uih)
{
    int i;
    const struct dialog *di = menu_getdialog(uih, item);
    for (i = 0; di[i].question; i++) {
	menu_freeparam(d + i, di + i);
    }
    free(d);

}

void
menu_activate(const menuitem * item, struct uih_context *c,
	      dialogparam * d)
{
    if (c == NULL
	&& (!(item->flags & MENUFLAG_ATSTARTUP) || firstqueue != NULL)) {
	menu_addqueue(item, d);
	return;
    } else {
	if (c != NULL && c->incalculation
	    && !(item->flags & MENUFLAG_INCALC)) {
	    if (c->flags & MENUFLAG_INTERRUPT)
		c->interrupt = 1;
	    menu_addqueue(item, d);
	    return;
	}
    }
    switch (item->type) {
    case MENU_SEPARATOR:
	x_error("separator activated!");
	break;
    case MENU_SUBMENU:
	x_error("submenu activated!");
	break;
    case MENU_NOPARAM:
	((void (*)(struct uih_context *)) item->function) (c);
	break;
    case MENU_INT:
	((void (*)(struct uih_context *, int)) item->function) (c,
								item->
								iparam);
	break;
    case MENU_STRING:
	((void (*)(struct uih_context *, const char *)) item->function) (c,
									 (const
									  char
									  *)
									 item->pparam);
	break;
    case MENU_DIALOG:
    case MENU_CUSTOMDIALOG:
	if (!menu_havedialog(item, c)) {
	    ((void (*)(struct uih_context * c, dialogparam *)) item->
	     function)
		(c, (dialogparam *) NULL);
	} else {
	    const menudialog *di = menu_getdialog(c, item);
	    if (di[0].question == NULL) {
		((void (*)(struct uih_context * c, dialogparam *))
		 item->function) (c, (dialogparam *) NULL);
		break;
	    } else if (di[1].question == NULL) {
		/*call function with right
		 *parameter. This avoids need to write wrappers*/
		switch (di[0].type) {
		case DIALOG_INT:
		case DIALOG_CHOICE:
		case DIALOG_ONOFF:
		    ((void (*)(struct uih_context * c, int)) item->
		     function) (c, d[0].dint);
		    break;
		case DIALOG_FLOAT:
		    ((void (*)(struct uih_context * c, number_t))
		     item->function) (c, d[0].number);
		    break;
		case DIALOG_COORD:
		    ((void (*)(struct uih_context * c, number_t, number_t))
		     item->function) (c, d[0].dcoord[0], d[0].dcoord[1]);
		    break;
		case DIALOG_STRING:
		case DIALOG_KEYSTRING:
		    ((void (*)(struct uih_context * c, char *)) item->
		     function)
			(c, d[0].dstring);
		    break;
		case DIALOG_IFILE:
		case DIALOG_OFILE:
		    ((void (*)(struct uih_context * c, xio_path))
		     item->function) (c, d[0].dpath);
		    break;
		default:
		    x_error("dialog:unknown type!");
		    break;
		}

	    } else
		((void (*)(struct uih_context * c, dialogparam *)) item->
		 function)
		    (c, d);
	}
	break;
    default:
	x_error("Menu_activate: unknown type %i!", item->type);
	break;
    }
}

int menu_enabled(const menuitem * item, struct uih_context *c)
{
    if (item->flags & (MENUFLAG_RADIO | MENUFLAG_CHECKBOX))
	switch (item->type) {
	case MENU_SEPARATOR:
	    return 0;
	case MENU_SUBMENU:
	case MENU_DIALOG:
	case MENU_NOPARAM:
	case MENU_CUSTOMDIALOG:
	    return (((int (*)(struct uih_context *)) item->control) (c));
	case MENU_INT:
	    return (((int (*)(struct uih_context *, int)) item->
		     control) (c, item->iparam));
	case MENU_STRING:
	    return (((int (*)(struct uih_context *, const char *)) item->
		     control)
		    (c, (const char *) item->pparam));
	default:
	    x_error("Menu_enabled: unknown type!");
	    break;
	}
    return 0;
}

void menu_delnumbered(int n, const char *name)
{
    menuitem *items;
    int i;
    char s[256];
    sprintf(s, "%s%i", name, 0);
    items = (menuitem *) menu_findcommand(s);
    menu_delete(items, n);
    for (i = 0; i < n; i++) {
	if (items[i].key)
	    free((char *) items[i].key);
	free((char *) items[i].shortname);
    }
    free(items);
}

const menuitem *menu_genernumbered(int n, const char *menuname,
				   const char *const * const names,
				   const char *keys, int type, int flags,
				   void (*function) (struct uih_context *
						     context, int),
				   int (*control) (struct uih_context *
						   context, int),
				   const char *prefix)
{
    int l = keys != NULL ? (int) strlen(keys) : -1;
    int i;
    menuitem *item = (menuitem *) malloc(sizeof(menuitem) * n);
    if (item == NULL)
	return NULL;
    for (i = 0; i < n; i++) {
	item[i].menuname = menuname;
	if (i < l) {
	    char *key = malloc(2);
	    item[i].key = key;
	    key[0] = keys[i];
	    key[1] = 0;
	} else
	    item[i].key = 0;
	item[i].type = type;
	item[i].flags = flags;
	item[i].iparam = i;
	item[i].name = names[i];
	item[i].shortname = names[i];
	if (prefix != NULL) {
	    char *shortname = (char *) malloc(strlen(prefix) + 4);
	    item[i].shortname = shortname;
	    sprintf(shortname, "%s%i", prefix, i);
	}
	item[i].function = (void (*)(void)) function;
	item[i].control = (int (*)(void)) control;
    }
    menu_add(item, n);
    return (item);
}

number_t menu_getfloat(const char *s, const char **error)
{
#ifdef HAVE_LONG_DOUBLE
    long double param = 0;
#else
    double param = 0;
#endif
#ifdef HAVE_LONG_DOUBLE
#ifndef USE_ATOLD
#ifdef USE_XLDIO
    param = x_strtold(s, NULL);
    if (0)
#else
    if (sscanf(s, "%LG", &param) == 0)
#endif
#else
    param = _atold(s);
    if (0)
#endif
    {
#else
    if (sscanf(s, "%lG", &param) == 0) {
#endif
	*error = "Floating point number expected";
	return 0;
    }
    return (param);
}

int menuparse_scheme = 0;
const char *menu_fillparam(struct uih_context *uih, tokenfunc f,
			   const menudialog * d, dialogparam * p)
{
    char *c = f(uih);
    const char *error = NULL;
    if (c == NULL)
	return "Parameter expected";
    switch (d->type) {
    case DIALOG_INT:
	if (sscanf(c, "%i", &p->dint) != 1) {
	    return "Integer parameter expected";
	}
	break;
    case DIALOG_FLOAT:
	p->number = menu_getfloat(c, &error);
	if (error != NULL)
	    return (error);
	break;
    case DIALOG_COORD:
	p->dcoord[0] = menu_getfloat(c, &error);
	if (error != NULL)
	    return (error);
	c = f(uih);
	if (c == NULL)
	    return "Imaginary part expected";
	p->dcoord[1] = menu_getfloat(c, &error);
	if (error != NULL)
	    return (error);
	break;
    case DIALOG_KEYSTRING:
	if (menuparse_scheme) {
	    if (c[0] != '\'')
		return "Key string parameter expected";
	    p->dstring = mystrdup(c + 1);
	} else
	    p->dstring = mystrdup(c);
	break;
    case DIALOG_STRING:
	if (menuparse_scheme) {
	    int l = strlen(c);
	    if (l < 2 || c[0] != '"' || c[l - 1] != '"')
		return "String parameter expected";
	    p->dstring = mystrdup(c + 1);
	    p->dstring[l - 2] = 0;
	} else
	    p->dstring = mystrdup(c);
	break;
    case DIALOG_IFILE:
    case DIALOG_OFILE:
	if (menuparse_scheme) {
	    int l = strlen(c);
	    if (l < 2 || c[0] != '"' || c[l - 1] != '"')
		return "String parameter expected";
	    p->dstring = mystrdup(c + 1);
	    p->dstring[l - 2] = 0;
	} else
	    p->dstring = mystrdup(c);
	break;
    case DIALOG_CHOICE:
	if (menuparse_scheme) {
	    if (c[0] != '\'')
		return "Key string parameter expected";
	    c++;
	}
	{
	    int i;
	    const char **keys = (const char **) d->defstr;
	    for (i = 0;; i++) {
		if (keys[i] == NULL)
		    return "Unknown parameter";
		if (!strcmp(c, keys[i])) {
		    p->dint = i;
		    return NULL;
		}
	    }
	}
    case DIALOG_ONOFF:
	if (menuparse_scheme) {
	    if (!strcmp("#t", c)) {
		p->dint = 1;
		return NULL;
	    }
	    if (!strcmp("#f", c)) {
		p->dint = 0;
		return NULL;
	    }
	} else {
	    if (!strcmp("on", c)) {
		p->dint = 1;
		return NULL;
	    }
	    if (!strcmp("off", c)) {
		p->dint = 0;
		return NULL;
	    }
	}
    default:
	x_error("Unknown dialog parameter type!");
    }				/*switch */
    return NULL;
}

static char errorstr[256];
const menuitem *menu_findcommand(const char *name)
{
    struct entry *entry;
    const menuitem *item;
    int len;
    len = strlen(name);
    if (len > 100)
	return NULL;
    entry = namehash[HASH(name, len)];
    while (entry != NULL) {
	if (!strcmp(entry->item->shortname, name))
	    break;
	entry = entry->nextname;
    }
    if (entry == NULL) {
	return NULL;
    }
    item = entry->item;
    return (item);
}

const char *menu_processcommand(struct uih_context *uih, tokenfunc f,
				int scheme, int mask, const char *root)
{
    char *c = f(uih);
    const menuitem *item;
    menuparse_scheme = scheme;
    if (c == NULL) {
	if (!menuparse_scheme)
	    return "Command name expected";
	return NULL;
    }
    item = menu_findcommand(c);
    if (item == NULL) {
	sprintf(errorstr, "%s:unknown function", c);
	return errorstr;
    }
    if (item->flags & mask) {
	sprintf(errorstr,
		"function '%s' not available in this context (%i, %i)", c,
		mask, item->flags);
	return errorstr;
    }
    if ((item->flags & MENUFLAG_NOPLAY) && uih != NULL) {
	if (root != NULL && !menu_available(item, root)) {
	    sprintf(errorstr, "function '%s' is disabled", c);
	    return errorstr;
	}
    }

    if ((item->flags & MENUFLAG_CHECKBOX) && scheme) {
	int w;
	c = f(uih);
	if (c == NULL) {
	    return ("Boolean parameter expected");
	}

	if (!strcmp("#t", c)) {
	    w = 1;
	} else if (!strcmp("#f", c)) {
	    w = 0;
	} else
	    return "Boolean parameter expected";
	if (w == menu_enabled(item, uih)) {
	    if (((w != 0) ^ ((item->flags & MENUFLAG_DIALOGATDISABLE) !=
			     0))
		|| (item->type != MENU_DIALOG
		    && item->type != MENU_CUSTOMDIALOG)) {
		return NULL;
	    } else
		menu_activate(item, uih, NULL);	/*disable it... */
	}
    }
    if (item->type != MENU_DIALOG && item->type != MENU_CUSTOMDIALOG) {
	menu_activate(item, uih, NULL);
	return NULL;
    }
    /*So we have some parameters */

    {
	dialogparam *param;
	const menudialog *d = menu_getdialog(uih, item);
	int i, n;
	for (n = 0; d[n].question != NULL; n++);
	param = (dialogparam *) malloc(n * sizeof(dialogparam));
	for (i = 0; i < n; i++) {
	    const char *error;
	    error = menu_fillparam(uih, f, d + i, param + i);
	    if (error != NULL) {
		sprintf(errorstr, "Function '%s' parameter %i:%s",
			item->shortname, i, error);
		for (n = 0; n < i; n++) {
		    menu_freeparam(param + i, d + i);
		}
		free(param);
		return errorstr;
	    }
	}
	menu_activate(item, uih, param);
	if (uih != NULL)
	    menu_destroydialog(item, param, uih);
    }
    return NULL;
}

static int argpos, margc;
static char **margv;
static int argposs;
static char *gettoken(struct uih_context *c)
{
    if (argpos == margc)
	return NULL;
    if (argpos == argposs) {
	if (!margv[argpos])
	    return NULL;
	return (margv[argpos++] + 1);
    } else
	return (margv[argpos++]);
}

int menu_processargs(int n, int argc, char **argv)
{
    const char *error;
    argpos = n;
    argposs = n;
    margc = argc;
    margv = argv;
    error =
	menu_processcommand(NULL, gettoken, 0, MENUFLAG_NOOPTION, "root");
    if (error) {
	x_error("%s", error);
	return -1;
    }
    return (argpos - 2);

}

void menu_printhelp(void)
{
    struct entry *e = firstitem;
    while (e) {
	if (e->item->type == MENU_SEPARATOR) {
	    e = e->next;
	    continue;
	}
	if (e->item->type == MENU_SUBMENU
	    && !(e->item->flags & MENUFLAG_NOOPTION)) {
	    struct entry *e1 = firstitem;
	    int n = 1;
	    while (e1) {
		/*if (e->item->type == MENU_SEPARATOR) {printf ("\n"); e1=e1->next;continue;} */
		if (e1->item->type != MENU_SUBMENU
		    && e1->item->type != MENU_SEPARATOR
		    && !(e1->item->flags & MENUFLAG_NOOPTION)
		    && !strcmp(e1->item->menuname, e->item->shortname)) {
		    if (n) {
			printf("\n%s\n\n", e->item->name);
			n = 0;
		    }
		    printf(" -%-15s", e1->item->shortname);
		    if (menu_havedialog(e1->item, NULL)) {
			const menudialog *d =
			    menu_getdialog(NULL, e1->item);
			while (d->question != NULL) {
			    switch (d->type) {
			    case DIALOG_INT:
				printf("integer ");
				break;
			    case DIALOG_FLOAT:
				printf("real_number ");
				break;
			    case DIALOG_COORD:
				printf("real_number  real_number ");
				break;
			    case DIALOG_KEYSTRING:
			    case DIALOG_STRING:
				printf("string ");
				break;
			    case DIALOG_IFILE:
				printf("input_file ");
				break;
			    case DIALOG_OFILE:
				printf("output_file ");
				break;
			    case DIALOG_CHOICE:
				{
				    int i;
				    const char **keys =
					(const char **) d->defstr;
				    for (i = 0;; i++) {
					if (keys[i] == NULL)
					    break;
					if (i != 0)
					    putc('|', stdout);
					printf("%s", keys[i]);
				    }
				    putc(' ', stdout);
				}
				break;
			    case DIALOG_ONOFF:
				printf("on|off ");
			    }
			    d++;
			}
			printf("\n%14s   ", "");
		    }
		    printf(" %s\n", e1->item->name);
		}
		e1 = e1->next;
	    }
	}
	e = e->next;
    }
}

void uih_xshlprintmenu(struct uih_context *c, const char *name)
{
    int i = 0;
    int nexti;
    const menuitem *item, *nextitem, *lastitem;
    int comma;
    printf("%%%s\n\n", name);
    printf("<menuhead><head>%s</head></menuhead>\n", menu_fullname(name));
    printf("<menuitems><center>\n");
    for (i = 0; (item = menu_item2(name, i)) != NULL; i++) {
	if (item->type == MENU_SEPARATOR)
	    printf("<p>\n");
	else if (item->type == MENU_SUBMENU)
	    printf("<p><submenu><a %s>%s</a>\n", item->shortname,
		   item->name);
	else
	    printf("<p><a %s>%s</a>\n", item->shortname, item->name);
    }
    printf("</center></menuitems>\n");
    lastitem = NULL;
    for (i = 0; (item = menu_item2(name, i)) != NULL; i++) {
	if (item->type == MENU_SEPARATOR)
	    continue;
	if (item->type != MENU_SUBMENU) {
	    for (nexti = i + 1;
		 (nextitem = menu_item2(name, nexti)) != NULL
		 && nextitem->type == MENU_SUBMENU; nexti++);
	    printf("<node %s, %s, %s, %s>\n", item->shortname,
		   (lastitem != NULL ? lastitem->shortname : ""),
		   nextitem != NULL ? nextitem->shortname : "", name);
	    printf("%%%s\n", item->shortname);
	    printf("<head>%s</head>\n", item->name);
	    if (!(item->flags & MENUFLAG_NOPLAY)) {
		printf("<p><emph>Syntax</emph>:(%s", item->shortname);
		if (item->flags & MENUFLAG_CHECKBOX)
		    printf(" bool");
		if (item->type == MENU_DIALOG
		    || item->type == MENU_CUSTOMDIALOG) {
		    int y;
		    const menudialog *di;
		    di = menu_getdialog(c, item);
		    if (item->flags & MENUFLAG_CHECKBOX)
			printf(" [");
		    for (y = 0; di[y].question != NULL; y++) {
			switch (di[y].type) {
			case DIALOG_INT:
			    printf(" integer");
			    break;
			case DIALOG_FLOAT:
			    printf(" float");
			    break;
			case DIALOG_STRING:
			    printf(" string");
			case DIALOG_KEYSTRING:
			    printf(" keyword");
			    break;
			case DIALOG_IFILE:
			    printf(" file");
			    break;
			case DIALOG_OFILE:
			    printf(" file");
			    break;
			case DIALOG_ONOFF:
			    printf(" bool");
			    break;
			case DIALOG_COORD:
			    printf(" complex");
			    break;
			case DIALOG_CHOICE:
			    printf(" keyword");
			    break;
			}
		    }
		    if (item->flags & MENUFLAG_CHECKBOX)
			printf(" ]");
		}
		printf(")\n");
	    }
	    printf("<p>\n<emph>Available as</emph>: ");
	    comma = 0;
	    if (!(item->flags & MENUFLAG_NOMENU))
		printf("menu item"), comma = 1;
	    if (!(item->flags & MENUFLAG_NOOPTION))
		printf("%scommand line option", comma ? ", " : ""), comma =
		    1;
	    if (!(item->flags & MENUFLAG_NOPLAY))
		printf("%scommand", comma ? ", " : ""), comma = 1;
	    printf("\n");
	    printf("\n");
	    lastitem = item;
	}
    }
}

void uih_xshlprintmenus(struct uih_context *c)
{
    struct entry *e = firstitem;
    struct entry *nexte;
    struct entry *laste;
    printf("%%menus\n");
    printf("<main><head>Menus</head></main>\n");
    printf("<menuitems><center>\n");
    while (e != NULL) {
	if (e->item->type == MENU_SUBMENU)
	    printf("<p><submenu><a %s>%s</a>\n", e->item->shortname,
		   e->item->name);
	e = e->next;
    }
    printf("</center></menuitems>\n");
    e = firstitem;
    laste = NULL;
    while (e != NULL) {
	if (e->item->type == MENU_SUBMENU) {
	    nexte = e->next;
	    while (nexte != NULL && nexte->item->type != MENU_SUBMENU)
		nexte = nexte->next;
	    printf("<node %s, %s, %s, %s>\n", e->item->shortname,
		   (laste != NULL ? laste->item->shortname : ""),
		   nexte != NULL ? nexte->item->shortname : "", "menus");
	    uih_xshlprintmenu(c, e->item->shortname);
	    laste = e;
	}
	e = e->next;
    }
    printf("%%endmenus");
}

void
menu_forall(struct uih_context *c,
	    void (*callback) (struct uih_context * c,
			      const menuitem * item))
{
    struct entry *e = firstitem;
    while (e != NULL) {
	callback(c, e->item);
	e = e->next;
    }
}
