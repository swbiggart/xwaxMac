//
//  Listing.m
//  xwaxMac
//
//  Created by Thomas Blench on 19/08/2009.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//
#include "library.h"
#include "listing.h"
#import "Listing.h"
#import "EyeTunes.h"
#import <Foundation/NSPathUtilities.h>



void iTunes_get_all_tracks(struct library_t *li) {
    
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];    
    EyeTunes *e = [EyeTunes sharedInstance];
    NSArray *tracks = [[e libraryPlaylist] tracks];
    
    if (tracks) {
        NSEnumerator *e = [tracks objectEnumerator];
        ETTrack *t = nil;
        while (t = [e nextObject]) {
            if ( ! [t location] ) continue; // thanks Jacques!
            NSURL *url = [NSURL URLWithString:[t location]];
            struct record_t record;
            strcpy(record.pathname, [[url path] UTF8String]);
            strcpy(record.artist, [[t artist] UTF8String]);
            strcpy(record.name, [[t name] UTF8String]);
            strcpy(record.title, [[t name] UTF8String]);
            library_add(li, &record);
        }
    }
}

int listing_match_itunes(struct listing_t *src, struct listing_t *dest, char *match)
{


    //
    fprintf(stderr, "Matching '%s'\n", match);

    
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];    
    EyeTunes *e = [EyeTunes sharedInstance];
    
    //NSArray *tracks = [[e libraryPlaylist] tracks];
    
    NSArray *tracks = [e search:[e libraryPlaylist] forString: [NSString stringWithCString:match] inField:kETSearchAttributeAll];
    if (tracks) {
        NSEnumerator *e = [tracks objectEnumerator];
        ETTrack *t = nil;
        while (t = [e nextObject]) {
            NSURL *url = [NSURL URLWithString:[t location]];

            struct record_t record;
            strcpy(record.pathname, [[url path] UTF8String]);
            strcpy(record.artist, [[t artist] UTF8String]);
            strcpy(record.name, [[t name] UTF8String]);
            strcpy(record.title, [[t name] UTF8String]);
            //            NSLog(@"> %@ %@", [t name], [t location]);
            if(listing_add(dest, &record) == -1)
                return -1;
        }
    }
    
    

    
    return 0;
}