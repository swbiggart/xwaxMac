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
    IBOutlet NSButton      *recordEnabledButton;
    IBOutlet NSPopUpButton *recordDevices;
    IBOutlet NSPopUpButton *recordDeviceLChan;
    IBOutlet NSPopUpButton *recordDeviceRChan;
    IBOutlet NSButton      *recordPathButton;
    IBOutlet NSTextField   *recordPath;
    IBOutlet NSPopUpButton *recordFormat;
    IBOutlet NSPopUpButton *recordBitrate;    
    IBOutlet NSPopUpButton *latency;
    IBOutlet NSPopUpButton *decks;
    IBOutlet NSPopUpButton *timecode;
    IBOutlet NSButton      *cancelButton;
    IBOutlet NSButton      *okButton;
    IBOutlet NSButton      *enabledButton;
    @public
    IBOutlet NSWindow *theWindow; // Preferences window
    
    struct prefs prefs;
    struct iopair *currentio;
    int returnCode;

}
// Callbacks on various UI elements
- (IBAction) inputChanged:(id)sender;
- (IBAction) outputChanged:(id)sender;
- (IBAction) deckChanged:(id)sender;
- (IBAction) deckEnabledChanged:(id)sender;
- (IBAction) inDeviceChanLChanged:(id)sender;
- (IBAction) inDeviceChanRChanged:(id)sender;
- (IBAction) outDeviceChanLChanged:(id)sender;
- (IBAction) outDeviceChanRChanged:(id)sender;
- (IBAction) recordPathButtonClicked:(id)sender;
- (IBAction) recordChanged:(id)sender;
- (IBAction) recordEnabledChanged:(id)sender;
- (IBAction) recordFormatChanged:(id)sender;
- (IBAction) okPressed:(id)sender;
- (IBAction) cancelPressed:(id)sender;
// Utility to populate lists
- (void) populateInputChannelList:(NSNumber*)deviceId;
- (void) populateOutputChannelList:(NSNumber*)deviceId;
- (void) populateRecordChannelList:(NSNumber*)deviceId;
@end
