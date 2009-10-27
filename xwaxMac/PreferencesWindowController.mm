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
    
    //populate defaults in ios so they are in sync with the first menu and have sensible defaults for 2nd and 3rd
    ios[0].inDeviceId = firstInDevId;
    strcpy(ios[0].inDeviceName, [[[inputDevices selectedItem] title] cStringUsingEncoding:NSASCIIStringEncoding]);
    ios[0].outDeviceId = firstOutDevId;
    strcpy(ios[0].outDeviceName, [[[outputDevices selectedItem] title] cStringUsingEncoding:NSASCIIStringEncoding]);
    ios[0].inDeviceChanL = 1;
    ios[0].inDeviceChanR = 2;
    ios[0].outDeviceChanL = 1;
    ios[0].outDeviceChanR = 2;
    ios[0].enabled = true;
    ios[1] = ios[0];
    ios[2] = ios[0];
    ios[2].enabled = false;
    currentio = &ios[0];
}

- (IBAction) okPressed:(id)sender
{
    int i,n;
    //write the preferences
    CFDictionaryRef dicts[3]; // for now we only support 3 decks

    CFStringRef latencyKey = CFSTR("latency");
    int l = [[[latency selectedItem] title] intValue];
    currentLatency = l;
    CFNumberRef latencyValue = CFNumberCreate(NULL, kCFNumberIntType, &l);
    CFPreferencesSetAppValue(latencyKey, latencyValue, kCFPreferencesCurrentApplication);

    CFStringRef notFirstTimeKey = CFSTR("notFirstTime");
    CFBooleanRef notFirstTimeValue = kCFBooleanTrue;
    CFPreferencesSetAppValue(notFirstTimeKey, notFirstTimeValue, kCFPreferencesCurrentApplication);

    CFStringRef timecodeKey = CFSTR("timecode");
    CFStringRef timecodeValue = (CFStringRef)[[timecode selectedItem] representedObject]  ;
    strcpy(currentTimecode, [[[timecode selectedItem] representedObject] cStringUsingEncoding:NSASCIIStringEncoding]);
    CFPreferencesSetAppValue(timecodeKey, timecodeValue, kCFPreferencesCurrentApplication);    
    
    //TODO prefs for this
    currentRecordDeviceId = [[[recordDevices selectedItem] representedObject] intValue];
    CFStringRef recordDeviceNameKey = CFSTR("recordDeviceName");
    CFStringRef recordDeviceNameValue =  (CFStringRef)[[recordDevices selectedItem] title];
    CFPreferencesSetAppValue(recordDeviceNameKey, recordDeviceNameValue, kCFPreferencesCurrentApplication);
    
    // Create a dictionary for each enabled deck
    for (i=0,n=0;i<3;i++) {
        if (!ios[i].enabled) {
            continue;
        }
        
        CFMutableDictionaryRef dict = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        dicts[n] = dict;
        n++;

        CFStringRef inDeviceIdKey = CFSTR("inDeviceName");
        CFStringRef inDeviceIdValue = CFStringCreateWithCString(kCFAllocatorDefault, ios[i].inDeviceName, kCFStringEncodingASCII);

        CFStringRef outDeviceIdKey = CFSTR("outDeviceName");
        CFStringRef outDeviceIdValue = CFStringCreateWithCString(kCFAllocatorDefault, ios[i].outDeviceName, kCFStringEncodingASCII);
    
        CFStringRef inDeviceChanLKey = CFSTR("inDeviceChanL");
        CFNumberRef inDeviceChanLValue = CFNumberCreate(NULL, kCFNumberIntType, &ios[i].inDeviceChanL);

        CFStringRef inDeviceChanRKey = CFSTR("inDeviceChanR");
        CFNumberRef inDeviceChanRValue = CFNumberCreate(NULL, kCFNumberIntType, &ios[i].inDeviceChanR);
    
        CFStringRef outDeviceChanLKey = CFSTR("outDeviceChanL");
        CFNumberRef outDeviceChanLValue = CFNumberCreate(NULL, kCFNumberIntType, &ios[i].outDeviceChanL);
    
        CFStringRef outDeviceChanRKey = CFSTR("outDeviceChanR");
        CFNumberRef outDeviceChanRValue = CFNumberCreate(NULL, kCFNumberIntType, &ios[i].outDeviceChanR);

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
    CFRelease(array);
    
    // Number of decks for the application to start up
    nDecks = n;
    
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

- (IBAction) deckChanged:(id)sender
{
    // re-populate with current values
    NSNumber *n = (NSNumber*)[[sender selectedItem] representedObject];
    currentio = &ios[[n intValue]];
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
        prefs->ios = wc->ios;
        prefs->latency = wc->currentLatency;
        prefs->nDecks = wc->nDecks;
        prefs->timecode = (char*)malloc(64*sizeof(char));
        strcpy(prefs->timecode, wc->currentTimecode);
        prefs->recordDeviceId = wc->currentRecordDeviceId;
    }
    return wc->returnCode;
}
