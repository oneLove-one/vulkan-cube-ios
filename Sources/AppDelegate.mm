#import "AppDelegate.h"

#import "CubeView.h"

@implementation AppDelegate

- (BOOL)application:(UIApplication*)application
    didFinishLaunchingWithOptions:(NSDictionary*)launchOptions {
    self.window = [[UIWindow alloc] initWithFrame:[UIScreen mainScreen].bounds];

    UIViewController* controller = [[UIViewController alloc] init];
    controller.view.backgroundColor = [UIColor blackColor];

    CubeView* cube = [[CubeView alloc] initWithFrame:controller.view.bounds];
    cube.translatesAutoresizingMaskIntoConstraints = NO;
    [controller.view addSubview:cube];

    [NSLayoutConstraint activateConstraints:@[
        [cube.leadingAnchor constraintEqualToAnchor:controller.view.leadingAnchor],
        [cube.trailingAnchor constraintEqualToAnchor:controller.view.trailingAnchor],
        [cube.topAnchor constraintEqualToAnchor:controller.view.topAnchor],
        [cube.bottomAnchor constraintEqualToAnchor:controller.view.bottomAnchor],
    ]];

    self.window.rootViewController = controller;
    [self.window makeKeyAndVisible];
    return YES;
}

@end
