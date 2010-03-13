//
//  ListingItem.h
//  xwaxMac
//
//  Created by Tom Blench on 30/01/2010.
//  Copyright 2010 Zen_Platypus. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface ListingItem : NSObject {
    NSString *title;
    NSString *artist;
    NSString *album;
}

- (NSString *)title;
- (void)setTitle:(NSString *)newTitle;

- (NSString *)artist;
- (void)setArtist:(NSString *)newArtist;

- (NSString *)album;
- (void)setAlbum:(NSString *)newAlbum;


@end
