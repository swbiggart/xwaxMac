//
//  LoadingWindowController.m
//  xwaxMac
//
//  Created by Tom Blench on 08/10/2009.
//  Copyright 2009 Zen_Platypus. All rights reserved.
//

#import "LoadingWindowController.h"


@implementation LoadingWindowController
- (void) hideLoadingWindow
{
    [theWindow orderOut:self];
}
- (void) showLoadingWindow
{
    [progress displayIfNeeded];
    [theWindow orderFront:self];
    lastUpdatedValue = 0.0f;
}
- (void) doUpdate:(float)percent
{
    [progress setDoubleValue:percent];
    if (percent - lastUpdatedValue > 1.0f) { 
        [progress display];
        lastUpdatedValue = percent;
    }
}

@end
// Load window and return pointer to new controller object
LoadingWindowController* loadLoadingWindow()
{
    LoadingWindowController *wc = [[LoadingWindowController alloc] init];
    [wc showLoadingWindow];
    
    if (![NSBundle loadNibNamed:@"Loading" owner:wc ]) {
        NSLog(@"Error loading Nib for document!");
        return 0;
    } else {
        NSLog(@"Loaded");
        NSLog(@"Returning %p", wc);
        // Run modal (ie suspend until dialog is dismissed)
        return wc;
    }
}
LoadingWindowController *wc;
// Plain C function to show loading dialog (called by xwax.c)
int showLoadingWindow()
{
    wc = loadLoadingWindow();
    if (!wc) {
        return -1;
    }
    return 0;
}
int hideLoadingWindow()
{
    [wc hideLoadingWindow];
    return 0;
}
int updateLoadingWindow(int n, int total)
{
    [wc doUpdate:(100.0f*((float)n)/((float)total))];
}