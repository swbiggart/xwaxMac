//
//  PreferencesWindowController.h
//  xwaxMac
//
//  Created by Thomas Blench on 08/09/2009.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include "ReadPreferences.h"



@interface PreferencesWindowController : NSWindow {

	IBOutlet NSPopUpButton *inputDevices;
	IBOutlet NSPopUpButton *inputDeviceLChan;
	IBOutlet NSPopUpButton *inputDeviceRChan;
	IBOutlet NSPopUpButton *outputDevices;
	IBOutlet NSPopUpButton *outputDeviceLChan;
	IBOutlet NSPopUpButton *outputDeviceRChan;
	IBOutlet NSPopUpButton *latency;
	IBOutlet NSPopUpButton *decks;
	IBOutlet NSPopUpButton *timecode;
	IBOutlet NSButton *cancelButton;
	IBOutlet NSButton *okButton;
	IBOutlet NSButton *enabledButton;
	@public
	bool wasOKPressed;
	IBOutlet NSWindow *theWindow;

	
	
	struct iopair ios[3];//for 3 decks
	struct iopair *currentio;
	int currentLatency;
	
}
- (IBAction) okPressed:(id)sender;
- (IBAction) inputChanged:(id)sender;
- (IBAction) outputChanged:(id)sender;
- (IBAction) deckChanged:(id)sender;
- (IBAction) deckEnabledChanged:(id)sender;
- (IBAction) inDeviceChanLChanged:(id)sender;
- (IBAction) inDeviceChanRChanged:(id)sender;
- (IBAction) outDeviceChanLChanged:(id)sender;
- (IBAction) outDeviceChanRChanged:(id)sender;
- (void) populateInputChannelList:(NSNumber*)deviceId;
- (void) populateOutputChannelList:(NSNumber*)deviceId;
@end
