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
#ifdef _plan9_
#include <u.h>
#include <libc.h>
#else
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#endif
#include <config.h>
#include <param.h>
#include <xmenu.h>
#include <version.h>
#include <xerror.h>
#define MAXPARAMS 40
static const struct params *params[40];
int nparams;

int params_parser(int argc, char **argv)
{
    int i, p = 0, d;
    int ie = 0;
    int is;
    const struct params *par = NULL;
    int error = 0;
    int found;
    for (i = 1; i < argc && !error; i++) {
	found = 0;
#ifdef MACOSX
	if (strncmp("-psn", argv[i], 4) == 0)
	    continue;
#endif
	if (!strcmp("-help", argv[i])) {
	    error = 1;
	    break;
	}
	for (d = 0; d < nparams; d++) {
	    par = params[d];
	    for (p = 0; par[p].name != NULL && !error; p++) {
		if (!strcmp(par[p].name, argv[i])) {
		    found = 1;
		    is = i;
		    switch (par[p].type) {
		    case P_SWITCH:
			*((int *) par[p].value) = 1;
			break;
		    case P_NUMBER:
			{
			    int n;
			    if (i == argc - 1) {
				x_error
				    ("parameter %s requires numeric value.",
				     argv[i]);
				error = 1;
				break;
			    }
			    if (sscanf(argv[i + 1], "%i", &n) != 1) {
				x_error("parameter for %s is not number.",
					argv[i]);
				error = 1;
				break;
			    }
			    *((int *) par[p].value) = n;
			    i++;
			}
			break;
		    case P_FLOAT:
			{
			    float n;
			    if (i == argc - 1) {
				x_error
				    ("parameter %s requires floating point numeric value.",
				     argv[i]);
				error = 1;
				break;
			    }
			    if (sscanf(argv[i + 1], "%f", &n) != 1) {
				x_error
				    ("parameter for %s is not floating point number.",
				     argv[i]);
				error = 1;
				break;
			    }
			    *((float *) par[p].value) = n;
			    i++;
			}
			break;
		    case P_STRING:
			{
			    if (i == argc - 1) {
				x_error
				    ("parameter %s requires string value.",
				     argv[i]);
				error = 1;
				break;
			    }
			    i++;
			    *((char **) par[p].value) = *(argv + i);
			}
		    }
		    ie = i;
		    i = is;
		}
	    }
	}
	if (d == nparams && !found) {
	    i = menu_processargs(i, argc, argv);
	    if (i < 0) {
		error = 1;
		break;
	    } else
		i++;
	} else
	    i = ie;
    }
    if (error) {
	const char *name[] = {
	    "",
	    "number",
	    "string",
	    "f.point"
	};
#ifndef _plan9_
	printf("                 XaoS" XaoS_VERSION " help text\n");
	printf
	    (" (This help is genereated automagically. I am sorry for all inconvencies)\n\n");
#endif
	printf("option string   param   description\n\n");
	for (d = 0; d < nparams; d++) {
	    par = params[d];
	    for (p = 0; par[p].name != NULL; p++) {
		if (par[p].type == P_HELP)
		    printf("\n%s\n\n", par[p].help);
		else if (!par[p].type)
		    printf(" %-14s   %s\n", par[p].name, par[p].help);
		else
		    printf(" %-14s  %s\n%14s    %s\n", par[p].name,
			   name[par[p].type], "", par[p].help);
	    }
	    if (p == 0)
		printf(" No options avaiable for now\n");
	}
	menu_printhelp();
	return 0;
    }
    return (1);
}

void params_register(const struct params *par)
{
    params[nparams++] = par;
}
