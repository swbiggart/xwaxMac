//
//  Listing.m
//  xwaxMac
//
//  Created by Thomas Blench on 19/08/2009.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "Listing.h"
#import "EyeTunes/EyeTunes.h"
#import <Foundation/NSPathUtilities.h>
#include "library.h"


void test_get_tracks_by_search(struct library_t *li) {
	NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];	
	EyeTunes *e = [EyeTunes sharedInstance];
	
	NSArray *tracks = [[e libraryPlaylist] tracks];
	
//	NSArray *tracks = [e search:[e libraryPlaylist] forString:@"" inField:kETSearchAttributeAll];
	if (tracks) {
		NSEnumerator *e = [tracks objectEnumerator];
		ETTrack *t = nil;
		while (t = [e nextObject]) {
			NSURL *url = [NSURL URLWithString:[t location]];
			NSString *str = [url path];
			struct record_t record;
			strcpy(record.pathname, [[url path] cString]);
			strcpy(record.artist, [[t artist] cString]);
			strcpy(record.name, [[t name] cString]);
			strcpy(record.title, [[t name] cString]);
			NSLog(@"> %@ %@", [t name], [t location]);
			library_add(li, &record);
		}
	}
}
