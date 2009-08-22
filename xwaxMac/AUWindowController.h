//
//  AUWindowController.h
//  xwaxMac
//
//  Created by Tom Blench on 22/08/2009.
//  Copyright 2009 Zen_Platypus. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <AudioUnit/AUCocoaUIView.h>
#import <CoreAudioKit/AUGenericView.h>

@interface AUWindowController : NSWindowController {
	// Container for AU View
    IBOutlet NSBox *				uiAUViewContainer;
	// Post-nib view manufacturing
    NSScrollView *					mScrollView;


}

- (void)showCocoaViewForAU:(AudioUnit)inAU;
+ (BOOL)plugInClassIsValid:(Class) pluginClass;
@end

AUWindowController* loadNib();