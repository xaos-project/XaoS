//
//  main.m
//  XaoS
//
//  Created by   on 5/22/13.
//
//

#import "AppDelegate.h"

int gargc;
char** gargv;

int main(int argc, char *argv[])
{
    gargc = argc;
    gargv = argv;
    
    
    setenv("LANG", ((NSString*)[[NSLocale preferredLanguages] objectAtIndex:0]).UTF8String, 1);
    
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
    }
}
