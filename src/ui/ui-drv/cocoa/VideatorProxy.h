/*
 *     XaoS, a fast portable realtime fractal zoomer 
 *                  Copyright (C) 1996 by
 *
 *      Jan Hubicka          (hubicka@paru.cas.cz)
 *      Thomas Marsh         (tmarsh@austin.ibm.com)
 *
 *    Cocoa Driver by J.B. Langston III (jb-langston@austin.rr.com)
 *    Videator Support by Andrew Stone (Stone Design)
 *    For information about Videator, visit http://www.stone.com/Videator
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
#ifdef VIDEATOR_SUPPORT
#import <Cocoa/Cocoa.h>

@ interface VideatorProxy:NSObject {
    BOOL _wantsThrottle;
    NSDate *_lastFrameCreatedDate;
    id _videatorProxy;
    NSCalendarDate *_killDate;
    BOOL _videatorEnabled;
}

-(void) sendImageRep:(NSBitmapImageRep *) imageRep;
-(void) toggleVideator:(id) sender;
-(BOOL) videatorEnabled;

@end
#endif
