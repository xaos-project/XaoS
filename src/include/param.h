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
#ifndef PARAMS_H
#define PARAMS_H

#ifdef __cplusplus
extern "C"
{
#endif

  struct params
  {
    CONST char *name;
    int type;
    void *value;
    CONST char *help;
  };

#define P_SWITCH 0
#define P_NUMBER 1
#define P_STRING 2
#define P_FLOAT 3
#define P_HELP 4

  int params_parser (int, char **);
  void params_register (CONST struct params *par);

#ifdef __cplusplus
}
#endif
#endif
