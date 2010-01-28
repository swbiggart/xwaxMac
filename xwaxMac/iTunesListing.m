//
//  Listing.m
//  xwaxMac
//
//  Created by Thomas Blench on 19/08/2009.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#include "library.h"
#include "listing.h"

#import "EyeTunes.h"
#import <Foundation/NSPathUtilities.h>
#include "InterfaceC.h"
#include "TrieMatcherC.h"

void iTunes_get_all_tracks(struct library_t *li) {
    
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];    
    EyeTunes *e = [EyeTunes sharedInstance];
    NSArray *tracks = [[e libraryPlaylist] tracks];
    
    if (tracks) {
        NSEnumerator *e = [tracks objectEnumerator];
        int nTracks = [tracks count];
        int i=0;
        ETTrack *t = nil;
        
        while (t = [e nextObject]) {
            if ( ! [t location] ) continue; // thanks Jacques!
            
            NSURL *url = [NSURL URLWithString:[t location]];
            
            struct record_t *record = malloc(sizeof(struct record_t));
            record->pathname[0] = '\0';
            record->artist[0] = '\0';
            record->title[0] = '\0';
            
            //TODO ALBUM
            strncpy(record->pathname, [[url path] UTF8String], MAX_PATHNAME);
            strncpy(record->artist, [[t artist] UTF8String], MAX_ARTIST);
            strncpy(record->title, [[t name] UTF8String], MAX_TITLE);
            strncpy(record->album, [[t album] UTF8String], MAX_ALBUM);
            
            // Index by title, and album/artist if present
            TrieMatcherAdd(record->title, record); // will always have title
            if (strlen(record->artist) != 0) {TrieMatcherAdd(record->artist, record);}            
            if (strlen(record->album) != 0) {TrieMatcherAdd(record->album, record);}
            
            // Placeholders if album/artist not present
            if(strlen(record->artist)==0){strcpy(record->artist, "<blank>");}
            if(strlen(record->album)==0){strcpy(record->album, "<blank>");}
            
            updateLoadingWindow(++i, nTracks);
        }
    }
    // No music.  The user may wish to continue (?)
    else 
    {
        [[NSAlert alertWithMessageText:nil
                         defaultButton:nil 
                       alternateButton:nil 
                           otherButton:nil 
             informativeTextWithFormat:@"No music found!\nxWax needs iTunes to be running so that it can access your music library.\n" ] runModal];
    }
}


