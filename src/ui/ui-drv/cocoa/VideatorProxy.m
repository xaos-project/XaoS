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
#include <aconfig.h>
#ifdef VIDEATOR_SUPPORT
#import "VideatorProxy.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <libc.h>

NSString *cheapHostName() {
    NSString *host = @"localhost";
    char s[_POSIX_HOST_NAME_MAX+1];
    s[0] = '\0';
    if (gethostname(s, _POSIX_HOST_NAME_MAX) == 0) {
        if (strlen(s) > 0)
            host = [[[NSString alloc] initWithUTF8String:s]autorelease];
    }
    return host;
}

#define VideatorServer	([NSString stringWithFormat:@"VideatorServer-%@",cheapHostName()])
#define FRAME_REFRESH_THRESHOLD  0.04

@protocol VideatorVendedProtocol
// to notify in main thread
- (void)runUpdateAlert:(NSString *)latestVersionNumber;
// XaoS
- (BOOL)wantsXaoSImage;
- (void)setXaosImageData:(NSData *)bmData;
// automator mode to check its not hung
- (BOOL)heartBeat;
@end


@implementation VideatorProxy

+ (void)setupDefaults
{
    NSString *userDefaultsValuesPath;
    NSDictionary *userDefaultsValuesDict;
    NSDictionary *initialValuesDict;
    NSArray *resettableUserDefaultsKeys;
    
    userDefaultsValuesPath=[[NSBundle mainBundle] pathForResource:@"UserDefaults" 
                                                           ofType:@"plist"];
    userDefaultsValuesDict=[NSDictionary dictionaryWithContentsOfFile:userDefaultsValuesPath];
    
    [[NSUserDefaults standardUserDefaults] registerDefaults:userDefaultsValuesDict];
    
    resettableUserDefaultsKeys=[NSArray arrayWithObjects:@"EnableVideator",nil];
    initialValuesDict=[userDefaultsValuesDict dictionaryWithValuesForKeys:resettableUserDefaultsKeys];
    
    [[NSUserDefaultsController sharedUserDefaultsController] setInitialValues:initialValuesDict];
}

+ (void)initialize {
    [self setupDefaults];
}

- (id)init {
    self = [super init];
    if (self) {
        _videatorEnabled = [[NSUserDefaults standardUserDefaults] boolForKey:@"EnableVideator"];
    }
    return self;
}

- (void)toggleVideator:(id)sender {
    _videatorEnabled ^= 1;
    [[NSUserDefaults standardUserDefaults] setBool:_videatorEnabled forKey:@"EnableVideator"];
}

- (BOOL)videatorEnabled {
    return _videatorEnabled;
}

- (void)connectionDidDie:(NSNotification *)n {
    _videatorProxy = nil;
    _killDate = [[NSCalendarDate date] retain];
    NSLog(@"Videator is dead... ...Long Live Videator!");
}

- (void)getProxy {
    
    // do not try and reconnect to an application that is terminating:
    if (_killDate && [_killDate timeIntervalSinceNow] > -10.0) return;
    else _killDate = nil;
    
    _videatorProxy = [[NSConnection rootProxyForConnectionWithRegisteredName:VideatorServer host:nil] retain];
    // if we can't find it, no big deal:
    if (_videatorProxy != nil) {
        [_videatorProxy setProtocolForProxy:@protocol(VideatorVendedProtocol)];
        [[NSDistributedNotificationCenter defaultCenter] addObserver:self selector:@selector(connectionDidDie:) name:@"VideatorWillTerminate" object:nil];
    }
}

- (void)sendImageRep:(NSBitmapImageRep *)imageRep {
    // simply return if user does not want this
    if (!_videatorEnabled) return;
    
    // Andrew's Videator hook - costs almost nothing since the view maintains the bitmapImageRep in hand -
    // We call it here because other mechanisms might cause a redraw in the view but we don't want that overhead
    // unless we've been notified that there really was a change:
    // HOWEVER we can only shove so much stuff down the pipe - let's try 30 frames per second threshold
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSDate *now = [NSDate date];
    
    
    if (_wantsThrottle) {
        static float _FRAME_REFRESH_THRESHOLD = 0.0;
        if (_FRAME_REFRESH_THRESHOLD == 0.0) {
            _FRAME_REFRESH_THRESHOLD = [[NSUserDefaults standardUserDefaults] floatForKey:@"RefreshThreshold"];
            if (_FRAME_REFRESH_THRESHOLD == 0.0) _FRAME_REFRESH_THRESHOLD = FRAME_REFRESH_THRESHOLD;
        }
        if (_lastFrameCreatedDate && [now timeIntervalSinceDate:_lastFrameCreatedDate] < _FRAME_REFRESH_THRESHOLD) return;
        [_lastFrameCreatedDate release];
        _lastFrameCreatedDate = [now retain];
    }
    
    if (!_videatorProxy) [self getProxy];
    NS_DURING
    if (_videatorProxy!=nil /* DO NOT WAIT FOR THE ROUNDTRIP && [_videatorProxy wantsXaoSImage] */)
        [_videatorProxy setXaosImageData:[imageRep TIFFRepresentationUsingCompression:NSTIFFCompressionLZW factor:0]];
    NS_HANDLER
    
    NS_ENDHANDLER
    
    [pool release];
}

@end
#endif