//
//  PreferencesWindowController.m
//  xwaxMac
//
//  Created by Thomas Blench on 08/09/2009.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "PreferencesWindowController.h"
#include "AudioDeviceList.h"
#include "AudioDevice.h"
// Need to know timecode definitions to populate list
extern "C"
{
#include "timecoder.h"
}

@implementation PreferencesWindowController

- (id)init
{
    self = [super init];
    return self;
}
- (void)awakeFromNib
{
    AudioDeviceList inputs(true);
    AudioDeviceList outputs(false);
    AudioDeviceList::DeviceList::iterator it;
    bool first;
    int firstInDevId;
    int firstOutDevId;
    int firstRecDevId;

    //populate inputs
    first = true;
    for(it = inputs.GetList().begin(); it != inputs.GetList().end(); it++) {
        NSMenuItem *newItem = [[NSMenuItem allocWithZone:[NSMenu menuZone]] initWithTitle:[NSString stringWithCString:(*it).mName] action:NULL keyEquivalent:@""];
        [newItem setRepresentedObject:[NSNumber numberWithInt:(*it).mID]];
        [[inputDevices menu] addItem:newItem];
        if (first) {
            [self populateInputChannelList:[newItem representedObject]];
            firstInDevId = (*it).mID;
            first = false;
        }
    }
    //populate outputs
    first = true;
    for(it = outputs.GetList().begin(); it != outputs.GetList().end(); it++) {
        NSMenuItem *newItem = [[NSMenuItem allocWithZone:[NSMenu menuZone]] initWithTitle:[NSString stringWithCString:(*it).mName] action:NULL keyEquivalent:@""];
        [newItem setRepresentedObject:[NSNumber numberWithInt:(*it).mID]];
        [[outputDevices menu] addItem:newItem];        
        if (first) {
            firstOutDevId = (*it).mID;
            [self populateOutputChannelList:[newItem representedObject]];
            first = false;
        }
    }
    //populate record devs
    first = true;
    for(it = inputs.GetList().begin(); it != inputs.GetList().end(); it++) {
        NSMenuItem *newItem = [[NSMenuItem allocWithZone:[NSMenu menuZone]] initWithTitle:[NSString stringWithCString:(*it).mName] action:NULL keyEquivalent:@""];
        [newItem setRepresentedObject:[NSNumber numberWithInt:(*it).mID]];
        [[recordDevices menu] addItem:newItem];  
        if (first) {
            firstRecDevId = (*it).mID;
            [self populateRecordChannelList:[newItem representedObject]];
            first = false;
        }
    }
    // deck numbers
    int n = [decks numberOfItems];
    int i;
    for (i = 0;i<n;i++)
    {
        [[decks itemAtIndex:i] setRepresentedObject:[NSNumber numberWithInt:i]];
    }
    
    //timecode defns
    struct timecode_def_t tc;
    for (i=0;;i++) {
        tc = get_all_definitions()[i];
        if (tc.name == NULL) {
            break;
        }
        NSMenuItem *newItem = [[NSMenuItem allocWithZone:[NSMenu menuZone]] initWithTitle:[NSString stringWithCString:tc.desc] action:NULL keyEquivalent:@""];
        [newItem setRepresentedObject:[NSString stringWithCString:tc.name]];
        [[timecode menu] addItem:newItem];        
    }
    
    //record path default
    NSString *path = @"~";
    NSString *standardizedPath = [path stringByStandardizingPath];
    [recordPath setStringValue:standardizedPath];
    strncpy(self->prefs.recordPath,[standardizedPath cStringUsingEncoding:kCFStringEncodingUTF8],1024);	    
    
    //populate defaults in ios so they are in sync with the first menu and have sensible defaults for 2nd and 3rd
    self->prefs.ios = (struct iopair*)malloc(3*sizeof(struct iopair));// FIXME this should just be a fixed array
    self->prefs.ios[0].inDeviceId = firstInDevId;
    strcpy(self->prefs.ios[0].inDeviceName, [[[inputDevices selectedItem] title] cStringUsingEncoding:NSASCIIStringEncoding]);
    self->prefs.ios[0].outDeviceId = firstOutDevId;
    strcpy(self->prefs.ios[0].outDeviceName, [[[outputDevices selectedItem] title] cStringUsingEncoding:NSASCIIStringEncoding]);
    self->prefs.ios[0].inDeviceChanL = 1;
    self->prefs.ios[0].inDeviceChanR = 2;
    self->prefs.ios[0].outDeviceChanL = 1;
    self->prefs.ios[0].outDeviceChanR = 2;
    self->prefs.ios[0].enabled = true;
    self->prefs.ios[1] = self->prefs.ios[0];
    self->prefs.ios[2] = self->prefs.ios[0];
    self->prefs.ios[2].enabled = false;
    currentio = &prefs.ios[0];
}

- (IBAction) okPressed:(id)sender
{
    int i,n;
    //write the preferences
    CFDictionaryRef dicts[3]; // for now we only support 3 decks

    CFStringRef latencyKey = CFSTR("latency");
    int l = [[[latency selectedItem] title] intValue];
    self->prefs.latency = l;
    CFNumberRef latencyValue = CFNumberCreate(NULL, kCFNumberIntType, &l);
    CFPreferencesSetAppValue(latencyKey, latencyValue, kCFPreferencesCurrentApplication);

    CFStringRef notFirstTimeKey = CFSTR("notFirstTime");
    CFBooleanRef notFirstTimeValue = kCFBooleanTrue;
    CFPreferencesSetAppValue(notFirstTimeKey, notFirstTimeValue, kCFPreferencesCurrentApplication);

    CFStringRef timecodeKey = CFSTR("timecode");
    CFStringRef timecodeValue = (CFStringRef)[[timecode selectedItem] representedObject];
    strcpy(self->prefs.timecode, [[[timecode selectedItem] representedObject] cStringUsingEncoding:NSASCIIStringEncoding]);
    CFPreferencesSetAppValue(timecodeKey, timecodeValue, kCFPreferencesCurrentApplication);    
    
    // TODO - copy all these values into the prefs struct
    
    self->prefs.recordDeviceId = [[[recordDevices selectedItem] representedObject] intValue];
    CFStringRef recordDeviceNameKey = CFSTR("recordDeviceName");
    CFStringRef recordDeviceNameValue =  (CFStringRef)[[recordDevices selectedItem] title];
    CFPreferencesSetAppValue(recordDeviceNameKey, recordDeviceNameValue, kCFPreferencesCurrentApplication);
    
    CFStringRef recordDeviceChanLKey = CFSTR("recordDeviceChanL");
    int cl = [[[recordDeviceLChan selectedItem] representedObject] intValue];
    self->prefs.recordDeviceChanL = cl;
    CFNumberRef recordDeviceChanLValue = CFNumberCreate(NULL, kCFNumberIntType, &cl);
    CFPreferencesSetAppValue(recordDeviceChanLKey, recordDeviceChanLValue, kCFPreferencesCurrentApplication);
    
    CFStringRef recordDeviceChanRKey = CFSTR("recordDeviceChanR");
    int cr = [[[recordDeviceRChan selectedItem] representedObject] intValue];
    self->prefs.recordDeviceChanL = cr;
    CFNumberRef recordDeviceChanRValue = CFNumberCreate(NULL, kCFNumberIntType, &cr);
    CFPreferencesSetAppValue(recordDeviceChanRKey, recordDeviceChanRValue, kCFPreferencesCurrentApplication);
    
    CFStringRef recordFormatKey = CFSTR("recordFormat");
    CFStringRef recordFormatValue = (CFStringRef)[[recordFormat selectedItem] title];
    strncpy(self->prefs.recordFormat, [[[recordFormat selectedItem] title] cStringUsingEncoding:NSUTF8StringEncoding], 64);
    CFPreferencesSetAppValue(recordFormatKey, recordFormatValue, kCFPreferencesCurrentApplication);
    
    CFStringRef recordBitrateKey = CFSTR("recordBitrate");
    int br = [[[recordBitrate selectedItem] title] intValue];
    self->prefs.recordBitrate = br;
    CFNumberRef recordBitrateValue = CFNumberCreate(NULL, kCFNumberIntType, &br);
    CFPreferencesSetAppValue(recordBitrateKey, recordBitrateValue, kCFPreferencesCurrentApplication);
    
    CFStringRef recordPathKey = CFSTR("recordPath");
    CFStringRef recordPathValue = (CFStringRef)[recordPath stringValue];
    strncpy(self->prefs.recordPath, [[recordPath stringValue] cStringUsingEncoding:NSUTF8StringEncoding], 1024);
    CFPreferencesSetAppValue(recordPathKey, recordPathValue, kCFPreferencesCurrentApplication);
    
    CFStringRef  recordEnabledKey = CFSTR("recordEnabled");
    CFBooleanRef recordEnabledValue = [recordEnabledButton state] == NSOnState ? kCFBooleanTrue : kCFBooleanFalse;
    CFPreferencesSetAppValue(recordEnabledKey, recordEnabledValue, kCFPreferencesCurrentApplication);
    
    // Create a dictionary for each enabled deck
    for (i=0,n=0;i<3;i++) {
        if (!self->prefs.ios[i].enabled) {
            continue;
        }
        
        CFMutableDictionaryRef dict = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        dicts[n] = dict;
        n++;

        CFStringRef inDeviceIdKey = CFSTR("inDeviceName");
        CFStringRef inDeviceIdValue = CFStringCreateWithCString(kCFAllocatorDefault, self->prefs.ios[i].inDeviceName, kCFStringEncodingASCII);

        CFStringRef outDeviceIdKey = CFSTR("outDeviceName");
        CFStringRef outDeviceIdValue = CFStringCreateWithCString(kCFAllocatorDefault, self->prefs.ios[i].outDeviceName, kCFStringEncodingASCII);
    
        CFStringRef inDeviceChanLKey = CFSTR("inDeviceChanL");
        CFNumberRef inDeviceChanLValue = CFNumberCreate(NULL, kCFNumberIntType, &self->prefs.ios[i].inDeviceChanL);

        CFStringRef inDeviceChanRKey = CFSTR("inDeviceChanR");
        CFNumberRef inDeviceChanRValue = CFNumberCreate(NULL, kCFNumberIntType, &self->prefs.ios[i].inDeviceChanR);
    
        CFStringRef outDeviceChanLKey = CFSTR("outDeviceChanL");
        CFNumberRef outDeviceChanLValue = CFNumberCreate(NULL, kCFNumberIntType, &self->prefs.ios[i].outDeviceChanL);
    
        CFStringRef outDeviceChanRKey = CFSTR("outDeviceChanR");
        CFNumberRef outDeviceChanRValue = CFNumberCreate(NULL, kCFNumberIntType, &self->prefs.ios[i].outDeviceChanR);

        CFDictionarySetValue(dict, inDeviceIdKey, inDeviceIdValue);
        CFDictionarySetValue(dict, outDeviceIdKey, outDeviceIdValue);
        CFDictionarySetValue(dict, inDeviceChanLKey, inDeviceChanLValue);
        CFDictionarySetValue(dict, inDeviceChanRKey, inDeviceChanRValue);
        CFDictionarySetValue(dict, outDeviceChanLKey, outDeviceChanLValue);
        CFDictionarySetValue(dict, outDeviceChanRKey, outDeviceChanRValue);

        CFRelease(inDeviceIdKey);
        CFRelease(inDeviceIdValue);
        CFRelease(outDeviceIdKey);
        CFRelease(outDeviceIdValue);
        CFRelease(inDeviceChanLKey);
        CFRelease(inDeviceChanLValue);
        CFRelease(inDeviceChanRKey);
        CFRelease(inDeviceChanRValue);
        CFRelease(outDeviceChanLKey);
        CFRelease(outDeviceChanLValue);
        CFRelease(outDeviceChanRKey);
        CFRelease(outDeviceChanRValue);        
    }
    
    // Array of all decks
    CFArrayRef  array = CFArrayCreate( kCFAllocatorDefault, 
                          (const void **)dicts, 
                          n, 
                          &kCFTypeArrayCallBacks ); 
    for (i=0;i<n;i++) {
        CFRelease(dicts[i]);
    }
    CFStringRef decksKey = CFSTR("decks");
    CFPreferencesSetAppValue(decksKey, array, kCFPreferencesCurrentApplication);
    CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication);
    
    CFRelease(decksKey);
    CFRelease(latencyKey);
    CFRelease(latencyValue);
    CFRelease(notFirstTimeKey);
    CFRelease(notFirstTimeValue);
    CFRelease(timecodeKey);
    CFRelease(timecodeValue);
    CFRelease(recordDeviceNameKey);
    CFRelease(recordDeviceNameValue);
    CFRelease(recordDeviceChanLKey);
    CFRelease(recordDeviceChanLValue);
    CFRelease(recordDeviceChanRKey);
    CFRelease(recordDeviceChanRValue);
    CFRelease(recordFormatKey);
    CFRelease(recordFormatValue);
    CFRelease(recordBitrateKey);
    CFRelease(recordBitrateValue);
    CFRelease(recordPathKey);
    CFRelease(recordPathValue);
    CFRelease(recordEnabledKey);
    CFRelease(recordEnabledValue);

    CFRelease(array);
    
    // Number of decks for the application to start up
    self->prefs.nDecks = n;
    
    // Close dialog and return to normal event loop
    returnCode = 0;
    [theWindow orderOut:self];
    [[NSApplication sharedApplication] stopModal];
}

- (IBAction) cancelPressed:(id)sender
{
    // Close dialog and exit
    returnCode = -2;
    [theWindow orderOut:self];
    [[NSApplication sharedApplication] stopModal];
}

- (IBAction) inputChanged:(id)sender
{
    NSNumber *n = (NSNumber*)[[sender selectedItem] representedObject];
    [self populateInputChannelList:n];
    NSString *name = [[sender selectedItem] title];
    strcpy(currentio->inDeviceName, [name cStringUsingEncoding:NSASCIIStringEncoding]);
    currentio->inDeviceId = [n intValue];
}

- (IBAction) outputChanged:(id)sender
{
    NSNumber *n = (NSNumber*)[[sender selectedItem] representedObject];
    [self populateOutputChannelList:n];
    NSString *name = [[sender selectedItem] title];
    strcpy(currentio->outDeviceName, [name cStringUsingEncoding:NSASCIIStringEncoding]);
    currentio->outDeviceId = [n intValue];
}

- (IBAction) recordChanged:(id)sender
{
    NSNumber *n = (NSNumber*)[[sender selectedItem] representedObject];
    [self populateRecordChannelList:n];
    NSString *name = [[sender selectedItem] title];
}

- (IBAction) deckChanged:(id)sender
{
    // re-populate with current values
    NSNumber *n = (NSNumber*)[[sender selectedItem] representedObject];
    currentio = &self->prefs.ios[[n intValue]];
    [inputDevices selectItemAtIndex:[inputDevices indexOfItemWithRepresentedObject:[NSNumber numberWithInt:currentio->inDeviceId]]];
    [inputDeviceLChan selectItemAtIndex:[inputDeviceLChan indexOfItemWithRepresentedObject:[NSNumber numberWithInt:currentio->inDeviceChanL]]];
    [inputDeviceRChan selectItemAtIndex:[inputDeviceRChan indexOfItemWithRepresentedObject:[NSNumber numberWithInt:currentio->inDeviceChanR]]];
    [outputDevices selectItemAtIndex:[outputDevices indexOfItemWithRepresentedObject:[NSNumber numberWithInt:currentio->outDeviceId]]];
    [outputDeviceLChan selectItemAtIndex:[outputDeviceLChan indexOfItemWithRepresentedObject:[NSNumber numberWithInt:currentio->outDeviceChanL]]];
    [outputDeviceRChan selectItemAtIndex:[outputDeviceRChan indexOfItemWithRepresentedObject:[NSNumber numberWithInt:currentio->outDeviceChanR]]];
    [enabledButton setState:currentio->enabled ? NSOnState : NSOffState];
}

- (IBAction) deckEnabledChanged:(id)sender
{
    currentio->enabled = ([enabledButton state] == NSOnState);
}

- (IBAction) inDeviceChanLChanged:(id)sender
{
    NSNumber *n = (NSNumber*)[[sender selectedItem] representedObject];
    currentio->inDeviceChanL = [n intValue];
}

- (IBAction) inDeviceChanRChanged:(id)sender
{
    NSNumber *n = (NSNumber*)[[sender selectedItem] representedObject];
    currentio->inDeviceChanR = [n intValue];
}

- (IBAction) outDeviceChanLChanged:(id)sender
{
    NSNumber *n = (NSNumber*)[[sender selectedItem] representedObject];
    currentio->outDeviceChanL = [n intValue];
}

- (IBAction) outDeviceChanRChanged:(id)sender
{
    NSNumber *n = (NSNumber*)[[sender selectedItem] representedObject];
    currentio->outDeviceChanR = [n intValue];
}

- (IBAction) recordEnabledChanged:(id)sender
{
    self->prefs.recordEnabled = ([recordEnabledButton state] == NSOnState);
    [recordDevices setEnabled:self->prefs.recordEnabled];
    [recordDeviceLChan setEnabled:self->prefs.recordEnabled];
    [recordDeviceRChan setEnabled:self->prefs.recordEnabled];
    [recordPathButton setEnabled:self->prefs.recordEnabled];
    [recordFormat setEnabled:self->prefs.recordEnabled];
    [recordBitrate setEnabled:self->prefs.recordEnabled];
    
}

// FIXME these 3 are all copy and pastes
- (void) populateInputChannelList:(NSNumber*)deviceId
{
    AudioDevice d([deviceId intValue], true);
    int i;
    int num = d.CountChannels();
    int nDelete = [inputDeviceLChan numberOfItems];
    for (i = 0;i<nDelete;i++)
    {
        [inputDeviceLChan removeItemAtIndex:0];
        [inputDeviceRChan removeItemAtIndex:0];
    }
    for (i = 0; i<num; i++)
    {
        char title[8];
        sprintf(title,"%d",i+1);
        NSMenuItem *newItem = [[NSMenuItem allocWithZone:[NSMenu menuZone]] initWithTitle:[NSString stringWithCString:title] action:NULL keyEquivalent:@""];
        [newItem setRepresentedObject:[NSNumber numberWithInt:i+1]];
        [[inputDeviceLChan menu] addItem:newItem];        
        NSMenuItem *newItem2 = [[NSMenuItem allocWithZone:[NSMenu menuZone]] initWithTitle:[NSString stringWithCString:title] action:NULL keyEquivalent:@""];
        [newItem2 setRepresentedObject:[NSNumber numberWithInt:i+1]];
        [[inputDeviceRChan menu] addItem:newItem2];        
    }    
    // select right channel as 2 for input
    [inputDeviceRChan selectItemAtIndex:1];
}

- (void) populateOutputChannelList:(NSNumber*)deviceId
{
    AudioDevice d([deviceId intValue], false);
    int i;
    int num = d.CountChannels();
    int nDelete = [outputDeviceLChan numberOfItems];
    for (i = 0;i<nDelete;i++)
    {
        [outputDeviceLChan removeItemAtIndex:0];
        [outputDeviceRChan removeItemAtIndex:0];
    }
    for (i = 0; i<num; i++)
    {
        char title[8];
        sprintf(title,"%d",i+1);
        NSMenuItem *newItem = [[NSMenuItem allocWithZone:[NSMenu menuZone]] initWithTitle:[NSString stringWithCString:title] action:NULL keyEquivalent:@""];
        [newItem setRepresentedObject:[NSNumber numberWithInt:i+1]];
        [[outputDeviceLChan menu] addItem:newItem];        
        NSMenuItem *newItem2 = [[NSMenuItem allocWithZone:[NSMenu menuZone]] initWithTitle:[NSString stringWithCString:title] action:NULL keyEquivalent:@""];
        [newItem2 setRepresentedObject:[NSNumber numberWithInt:i+1]];
        [[outputDeviceRChan menu] addItem:newItem2];        
    }    
    // select right channel as 2 for output
    [outputDeviceRChan selectItemAtIndex:1];    
}

- (void) populateRecordChannelList:(NSNumber*)deviceId
{
    AudioDevice d([deviceId intValue], true);
    int i;
    int num = d.CountChannels();
    int nDelete = [recordDeviceLChan numberOfItems];
    for (i = 0;i<nDelete;i++)
    {
        [recordDeviceLChan removeItemAtIndex:0];
        [recordDeviceRChan removeItemAtIndex:0];
    }
    for (i = 0; i<num; i++)
    {
        char title[8];
        sprintf(title,"%d",i+1);
        NSMenuItem *newItem = [[NSMenuItem allocWithZone:[NSMenu menuZone]] initWithTitle:[NSString stringWithCString:title] action:NULL keyEquivalent:@""];
        [newItem setRepresentedObject:[NSNumber numberWithInt:i+1]];
        [[recordDeviceLChan menu] addItem:newItem];        
        NSMenuItem *newItem2 = [[NSMenuItem allocWithZone:[NSMenu menuZone]] initWithTitle:[NSString stringWithCString:title] action:NULL keyEquivalent:@""];
        [newItem2 setRepresentedObject:[NSNumber numberWithInt:i+1]];
        [[recordDeviceRChan menu] addItem:newItem2];        
    }    
    // select right channel as 2 for record
    [recordDeviceRChan selectItemAtIndex:1];    
}

// TODO check path accessibility

- (IBAction) recordPathButtonClicked:(id)sender
{
	NSOpenPanel* panel = [NSOpenPanel openPanel];
	[panel setCanChooseFiles:NO];
	[panel setCanChooseDirectories:YES];
	[panel setAllowsMultipleSelection:NO];
	
	if ([panel runModalForDirectory:@"~" file:nil types:nil] == NSOKButton)
	{
		NSLog(@"You selected %@", [panel filename]);
	}	
    [recordPath setStringValue:[panel filename]];
    strncpy(self->prefs.recordPath,[[panel filename] cStringUsingEncoding:kCFStringEncodingUTF8],1024);	
}

- (IBAction) recordFormatChanged:(id)sender
{
    NSString *t = [[sender selectedItem] title];
    if ([t isEqualToString:@"AAC"])
    {
        [recordBitrate setEnabled:TRUE];
    }
    else 
    {        
        [recordBitrate setEnabled:FALSE];
    }
}


@end

// Load window and return pointer to new controller object
PreferencesWindowController* loadPreferencesWindow()
{
    PreferencesWindowController *wc = [[PreferencesWindowController alloc] init];
    
    if (![NSBundle loadNibNamed:@"Preferences" owner:wc ]) {
        NSLog(@"Error loading Nib for document!");
        return 0;
    } else {
        NSLog(@"Loaded");
        NSLog(@"Returning %p", wc);
        // Run modal (ie suspend until dialog is dismissed)
        [[NSApplication sharedApplication] runModalForWindow:wc->theWindow];
        return wc;
    }
}

// Plain C function to get preferences from dialog (called by xwax.c)
int showPrefsWindow(struct prefs *prefs)
{
    PreferencesWindowController *wc = loadPreferencesWindow();
    if (!wc) {
        return -1;
    }
    if (wc->returnCode == 0) {
        // Read off settings once dialog has been dismissed
        /*
        prefs->ios = wc->ios;
        prefs->latency = wc->currentLatency;
        prefs->nDecks = wc->nDecks;
        strcpy(prefs->timecode, wc->currentTimecode);
        prefs->recordDeviceId = wc->currentRecordDeviceId;
         */
        memcpy(prefs,&wc->prefs,sizeof(struct prefs));
    }
    return wc->returnCode;
}
