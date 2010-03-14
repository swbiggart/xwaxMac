//
//  ListingItem.m
//  xwaxMac
//
//  Created by Tom Blench on 30/01/2010.
//  Copyright 2010 Zen_Platypus. All rights reserved.
//

#import "ListingItem.h"


@implementation ListingItem

- (id)init
{
    [super init];
    return self;
}

- (NSString *)title
{
    return title;
}

- (void)setTitle:(NSString *)newTitle
{
    [newTitle retain];
    [title release];
    title = newTitle;
}

- (NSString *)album
{
    return album;
}

- (void)setAlbum:(NSString *)newAlbum
{
    [newAlbum retain];
    [album release];
    album = newAlbum;
}

- (NSString *)artist
{
    return artist;
}

- (void)setArtist:(NSString *)newArtist
{
    [newArtist retain];
    [artist release];
    artist = newArtist;
}


@end
