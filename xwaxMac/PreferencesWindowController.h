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
    // Pop-up menus and buttons
    IBOutlet NSPopUpButton *inputDevices;
    IBOutlet NSPopUpButton *inputDeviceLChan;
    IBOutlet NSPopUpButton *inputDeviceRChan;
    IBOutlet NSPopUpButton *outputDevices;
    IBOutlet NSPopUpButton *outputDeviceLChan;
    IBOutlet NSPopUpButton *outputDeviceRChan;
    IBOutlet NSPopUpButton *recordDevices;
    IBOutlet NSPopUpButton *latency;
    IBOutlet NSPopUpButton *decks;
    IBOutlet NSPopUpButton *timecode;
    IBOutlet NSButton *cancelButton;
    IBOutlet NSButton *okButton;
    IBOutlet NSButton *enabledButton;
    @public
    IBOutlet NSWindow *theWindow; // Preferences window
    struct iopair ios[3]; // for 3 decks
    struct iopair *currentio;
    int currentLatency;
    int nDecks;
    char currentTimecode[64];
    int returnCode;
    int currentRecordDeviceId;
}
// Callbacks on various UI elements
- (IBAction) okPressed:(id)sender;
- (IBAction) cancelPressed:(id)sender;
- (IBAction) inputChanged:(id)sender;
- (IBAction) outputChanged:(id)sender;
- (IBAction) deckChanged:(id)sender;
- (IBAction) deckEnabledChanged:(id)sender;
- (IBAction) inDeviceChanLChanged:(id)sender;
- (IBAction) inDeviceChanRChanged:(id)sender;
- (IBAction) outDeviceChanLChanged:(id)sender;
- (IBAction) outDeviceChanRChanged:(id)sender;
// Utility to populate lists
- (void) populateInputChannelList:(NSNumber*)deviceId;
- (void) populateOutputChannelList:(NSNumber*)deviceId;
@end
