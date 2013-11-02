//
//  AppDelegate.m
//  XaoS
//
//  Created by   on 5/22/13.
//
//
#import "AppDelegate.h"
#import "ViewController.h"

@implementation AppDelegate

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    
    
    self.window = [[UIWindow alloc] initWithFrame:[UIScreen mainScreen].bounds];
    self.window.rootViewController = [AppViewController new];
    [self.window makeKeyAndVisible];
    
    return YES;
}
@end
