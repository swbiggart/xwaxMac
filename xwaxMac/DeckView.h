//
//  DeckView.h
//  xwaxMac
//
//  Created by Tom Blench on 17/11/2009.
//  Copyright 2009 Zen_Platypus. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface DeckView : NSView {
@public
    CGLayerRef overviewLayer;
}
- (CGContextRef)getContext;
@end

DeckView *dv; // for C access, very nasty