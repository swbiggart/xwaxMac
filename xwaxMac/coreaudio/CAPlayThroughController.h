/* Based on Apple sample code */
 
#import <Cocoa/Cocoa.h>
#include "CAPlayThrough.h"
#include "AudioDeviceList.h"

@interface CAPlayThroughController : NSObject
{
    IBOutlet NSPopUpButton *        mInputDevices;
    IBOutlet NSPopUpButton *        mOutputDevices;
    IBOutlet NSButton *                mStartButton;
    IBOutlet NSProgressIndicator *    mProgress;

    CAPlayThroughHost *                playThroughHost;
            
    AudioDeviceList *                mInputDeviceList;
    AudioDeviceList    *                mOutputDeviceList;
    AudioDeviceID                    inputDevice;
    AudioDeviceID                    outputDevice;
}

- (IBAction)startStop:(id)sender;
- (IBAction)inputDeviceSelected:(id)sender;
- (IBAction)outputDeviceSelected:(id)sender;

- (void)start: (id)sender;
- (void)stop: (id)sender;
- (void)resetPlayThrough;
@end
