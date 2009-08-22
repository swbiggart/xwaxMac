//
//  AUWindowController.mm
//  xwaxMac
//
//  Created by Tom Blench on 22/08/2009.
//  Copyright 2009 Zen_Platypus. All rights reserved.
//

#import "AUWindowController.h"

static id lastSelf = nil;

@implementation AUWindowController

- (id)init
{
	self = [super init];

	lastSelf = self;
	
	return self;
}

- (id)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        // Initialization code here.
    }
    return self;
}
/*
- (void)drawRect:(NSRect)rect {
    // Drawing code here.
}*/

- (void)awakeFromNib
{
	static int loaded=0;
	loaded++;
    NSLog(@"awake %p", self );
	if (loaded == 1)
	{
    NSLog(@"LoadNib");
    // create scroll-view
    NSRect frameRect = [[uiAUViewContainer contentView] frame];
    mScrollView = [[[NSScrollView alloc] initWithFrame:frameRect] autorelease];
    [mScrollView setDrawsBackground:NO];
    [mScrollView setHasHorizontalScroller:YES];
    [mScrollView setHasVerticalScroller:YES];
    [uiAUViewContainer setContentView:mScrollView];
    


    
	// make this the app. delegate
	[NSApp setDelegate:self];
	[[self window] setDelegate: self];
		[[self window] firstResponder];
	}
}

- (void)showCocoaViewForAU:(AudioUnit)inAU
{
	// get AU's Cocoa view property
	
	if (inAU->data[0] == 0)
	{
		NSLog(@"Null");
	}
	
    UInt32 						dataSize;
    Boolean 					isWritable;
    AudioUnitCocoaViewInfo *	cocoaViewInfo = NULL;
    UInt32						numberOfClasses;
    
    OSStatus result = AudioUnitGetPropertyInfo(	inAU,
											   kAudioUnitProperty_CocoaUI,
											   kAudioUnitScope_Global, 
											   0,
											   &dataSize,
											   &isWritable );
    
    numberOfClasses = (dataSize - sizeof(CFURLRef)) / sizeof(CFStringRef);
    
    NSURL 	 *	CocoaViewBundlePath = nil;
    NSString *	factoryClassName = nil;
    
	// Does view have custom Cocoa UI?
    if ((result == noErr) && (numberOfClasses > 0) ) {
        cocoaViewInfo = (AudioUnitCocoaViewInfo *)malloc(dataSize);
        if(AudioUnitGetProperty(		inAU,
								kAudioUnitProperty_CocoaUI,
								kAudioUnitScope_Global,
								0,
								cocoaViewInfo,
								&dataSize) == noErr) {
            CocoaViewBundlePath	= (NSURL *)cocoaViewInfo->mCocoaAUViewBundleLocation;
			
			// we only take the first view in this example.
            factoryClassName	= (NSString *)cocoaViewInfo->mCocoaAUViewClass[0];
        } else {
            if (cocoaViewInfo != NULL) {
				free (cocoaViewInfo);
				cocoaViewInfo = NULL;
			}
        }
    }
	
	NSView *AUView = nil;
	BOOL wasAbleToLoadCustomView = NO;
	
	// [A] Show custom UI if view has it
	if (CocoaViewBundlePath && factoryClassName) {
		NSBundle *viewBundle  	= [NSBundle bundleWithPath:[CocoaViewBundlePath path]];
		if (viewBundle == nil) {
			NSLog (@"Error loading AU view's bundle");
		} else {
			Class factoryClass = [viewBundle classNamed:factoryClassName];
			NSAssert (factoryClass != nil, @"Error getting AU view's factory class from bundle");
			
			// make sure 'factoryClass' implements the AUCocoaUIBase protocol
			NSAssert(	[AUWindowController plugInClassIsValid:factoryClass],
					 @"AU view's factory class does not properly implement the AUCocoaUIBase protocol");
			
			// make a factory
			id factoryInstance = [[[factoryClass alloc] init] autorelease];
			NSAssert (factoryInstance != nil, @"Could not create an instance of the AU view factory");
			// make a view
			AUView = [factoryInstance	uiViewForAudioUnit:inAU
												withSize:[[mScrollView contentView] bounds].size];
			
			// cleanup
			[CocoaViewBundlePath release];
			if (cocoaViewInfo) {
				UInt32 i;
				for (i = 0; i < numberOfClasses; i++)
					CFRelease(cocoaViewInfo->mCocoaAUViewClass[i]);
				
				free (cocoaViewInfo);
			}
			wasAbleToLoadCustomView = YES;
		}
	}
	
	if (!wasAbleToLoadCustomView) {
		// [B] Otherwise show generic Cocoa view
		AUView = [[AUGenericView alloc] initWithAudioUnit:inAU];
		printf("Bounds are %f %f\n", [AUView bounds].size.width, [AUView bounds].size.height );
		[(AUGenericView *)AUView setShowsExpertParameters:YES];
		[AUView autorelease];
    }
	
	// Display view
	NSRect viewFrame = [AUView frame];
	NSSize frameSize = [NSScrollView	frameSizeForContentSize:viewFrame.size
									   hasHorizontalScroller:[mScrollView hasHorizontalScroller]
										 hasVerticalScroller:[mScrollView hasVerticalScroller]
												  borderType:[mScrollView borderType]];
	
	NSRect newFrame;
	newFrame.origin = [mScrollView frame].origin;
	newFrame.size = frameSize;
	
	NSRect currentFrame = [mScrollView frame];
	[mScrollView setFrame:newFrame];
	[mScrollView setDocumentView:AUView];

	NSSize oldContentSize = [[[self window] contentView] frame].size;
	NSSize newContentSize = oldContentSize;
	newContentSize.width += (newFrame.size.width - currentFrame.size.width);
	newContentSize.height += (newFrame.size.height - currentFrame.size.height);
	
	[[self window] setContentSize:newContentSize];
	[mScrollView setNeedsDisplay:TRUE];
	[uiAUViewContainer setNeedsDisplay:TRUE];
}


+ (BOOL)plugInClassIsValid:(Class) pluginClass
{
	if ([pluginClass conformsToProtocol:@protocol(AUCocoaUIBase)]) {
		if ([pluginClass instancesRespondToSelector:@selector(interfaceVersion)] &&
			[pluginClass instancesRespondToSelector:@selector(uiViewForAudioUnit:withSize:)]) {
			return YES;
		}
	}
	
    return NO;
}
@end


AUWindowController* loadNib()
{
	AUWindowController *wc = [[AUWindowController alloc] init];

	if (![NSBundle loadNibNamed:@"AUWindow" owner:wc ]) {
		NSLog(@"Error loading Nib for document!");
		return 0;
	} else {
		NSLog(@"Loaded");
		NSLog(@"Returning %p", lastSelf);
		return lastSelf;
	}

}
