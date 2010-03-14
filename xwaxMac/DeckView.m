//
//  DeckView.m
//  xwaxMac
//
//  Created by Tom Blench on 17/11/2009.
//  Copyright 2009 Zen_Platypus. All rights reserved.
//

#import "DeckView.h"
#include "track.h"

struct rect_t {
    signed short x, y, w, h;
};


@implementation DeckView 

- (id)initWithFrame:(NSRect)frameRect
{
    self = [super initWithFrame:frameRect];
    overviewLayer = nil;
    return self;
}

- (void)drawRect:(NSRect)rect
{
    CGContextRef myContext = [[NSGraphicsContext // 1
                  currentContext]graphicsPort];
    // ********** Your drawing code here ********** // 2
    CGContextSetRGBFillColor (myContext, 1, 0, 0, 1);// 3
    CGContextFillRect (myContext, CGRectMake (0, 0, 200, 100 ));// 4
    CGContextSetRGBFillColor (myContext, 0, 0, 1, .5);// 5
    CGContextFillRect (myContext, CGRectMake (0, 0, 100, 200));// 6
}

- (CGContextRef)getContext
{
    CGContextRef myContext = [[NSGraphicsContext 
                               currentContext]graphicsPort];
    return myContext;
}

@end

// TODO poke in the deck etc structures needed to do drawing
void instantiateDeckView()
{
    dv = [[DeckView alloc] init];
    
    if (![NSBundle loadNibNamed:@"Deck" owner:dv ]) {
        NSLog(@"Error loading Nib for document!");
        return;
//        return 0;
    } else {
        NSLog(@"Loaded");
        NSLog(@"Returning %p", dv);
        return;
//        return dv;
    }
}


// TODO - lots of wrongness here - update the overview and big wave layers after loading and then composite them in the draw method
void deckView_draw_overview(const struct rect_t *rect,
                          struct track_t *tr, int position)
{
    int x, y, w, h, r, c, sp, fade, bytes_per_pixel, pitch, height,
    current_position;
//    Uint8 *pixels, *p;
//    SDL_Color col;
    
    CGColorRef col;
    CGColorRef background_col = CGColorCreateGenericRGB(0, 0, 0, 1);
    CGColorRef needle_col = CGColorCreateGenericRGB(0, 0, 1, 1);
    CGColorRef warn_col = CGColorCreateGenericRGB(1, 0, 0, 1);
    CGColorRef elapsed_col = CGColorCreateGenericRGB(0, 0, 0.5, 1);
    
    x = rect->x;
    y = rect->y;
    w = rect->w;
    h = rect->h;
    
    int METER_WARNING_TIME = 20 ;
    
    // 1st time layer stuff
    if (dv->overviewLayer == nil)
    {
        CGSize layerSize = CGSizeMake(rect->w, rect->h);
        dv->overviewLayer = CGLayerCreateWithContext([dv getContext], layerSize, NULL);
    }
    
    CGContextRef ctx = [dv->overviewLayer getContext];
    CGContextSetFillColorWithColor(ctx, col);

    
    if(tr->length)
        current_position = (long long)position * w / tr->length;
    else
        current_position = 0;
    
    for(c = 0; c < w; c++) {
        
        /* Collect the correct meter value for this column */
        
        sp = (long long)tr->length * c / w;
        
        if(sp < tr->length) /* account for rounding */
            height = track_get_overview(tr, sp) * h / 256;
        else
            height = 0;
        
        /* Choose a base colour to display in */
        
        if(!tr->length) {
            col = background_col;
            fade = 0;
        } else if(c == current_position) {
            col = needle_col;
            fade = 1;
        } else if(position > tr->length - tr->rate * METER_WARNING_TIME) {
            col = warn_col;
            fade = 3;
        } else {
            col = elapsed_col;
            fade = 3;
        }
        /*
        if(tr->status == TRACK_STATUS_IMPORTING) {
            col.b >>= 1;
            col.g >>= 1;
            col.r >>= 1;
        }
        
        if(c < current_position) {
            col.b >>= 1;
            col.g >>= 1;
            col.r >>= 1;
        }
        */
        /* Store a pointer to this column of the framebuffer */
        
//        p = pixels + y * pitch + (x + c) * bytes_per_pixel;

        // Todo - create/update a layer for later use when drawing
        CGContextFillRect(ctx, CGRectMake(x+c, y, 1, height));
        
    }
}