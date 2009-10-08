//
//  LoadingWindowController.h
//  xwaxMac
//
//  Created by Tom Blench on 08/10/2009.
//  Copyright 2009 Zen_Platypus. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface LoadingWindowController : NSWindow {
    IBOutlet NSProgressIndicator *progress;
    float lastUpdatedValue;
    @public     
    IBOutlet NSWindow *theWindow; // Loading window
}
- (void) hideLoadingWindow;
- (void) showLoadingWindow;
- (void) doUpdate:(float)percent;


@end
