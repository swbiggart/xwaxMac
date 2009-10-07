//
//  AppHelpers.m
//  xwaxMac
//
//  Created by Thomas Blench on 07/10/2009.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "AppHelpers.h"


@implementation AppHelpers
+ (char*) getBundleDir
{
    NSString *s = [[NSBundle mainBundle] bundlePath];
    return [s cString];
}

@end

char *getBundleDirC()
{
    return [AppHelpers getBundleDir];
}
