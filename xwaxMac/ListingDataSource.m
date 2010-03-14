//
//  ListingDataSource.m
//
//  Created by Tom Blench on 30/01/2010.
//  Copyright 2010 Man of Leisure. All rights reserved.
//

#import "ListingDataSource.h"
#import "ListingItem.h"
#include "listing.h"
#include "TrieMatcherC.h"

@implementation ListingDataSource

// TODO replace with real data interacting with our trie structure

- (id)init
{
    [super init];
    items = [[NSMutableArray alloc] init];
    ListingItem *i1 = [[ListingItem alloc] init];
    ListingItem *i2 = [[ListingItem alloc] init];
    ListingItem *i3 = [[ListingItem alloc] init];
    [i1 setAlbum:@"Siberian Nose Flute Classics of the 1950s"];
    [i1 setTitle:@"Haunted Basket"];
    [i1 setArtist:@"Rev J. W. Smedley-Sidley-Harbinger-Doom"];
    [i2 setTitle:@"Brian"];
    [i3 setTitle:@"The best of Pingu"];
    [items addObject:i1];
    [items addObject:i2];
    [items addObject:i3];
    return self;
}

- (int)numberOfRowsInTableView:(NSTableView *)tableView
{
    struct listing_t list;
    list.record = 0;
    TrieMatcherLookup("spoon", &list);
    return list.entries;
//    return [items count];
}

- (id)tableView:(NSTableView *)tableView 
objectValueForTableColumn:(NSTableColumn *)tableColumn
                      row:(int)row
{
    NSString *identifier = [tableColumn identifier];
    struct listing_t list;
    list.record = 0;
    TrieMatcherLookup("spoon", &list);
    ListingItem *item = list.record[row]->item;
    return [item valueForKey:identifier];
    
    
/*
    NSString *identifier = [tableColumn identifier];
    ListingItem *item = [items objectAtIndex:row];
    return [item valueForKey:identifier];
 */
}

@end
@implementation ListingTableView

// -------------------------------------------------------------------------------
//	controlTextDidChange:
//
//	The text in NSSearchField has changed, try to attempt type completion.
// -------------------------------------------------------------------------------
- (void)controlTextDidChange:(NSNotification *)obj
{
    NSTextView* textView = [[obj userInfo] objectForKey:@"NSFieldEditor"];
    /*
    if (!completePosting && !commandHandling)	// prevent calling "complete" too often
    {
        completePosting = YES;
        [textView complete:nil];
        completePosting = NO;
    }*/
}
@end
void instantiateTableView()
{
    ListingTableView *tv = [[ListingTableView alloc] init]; // look mom, we're on tv!
    
    if (![NSBundle loadNibNamed:@"ListingWindow" owner:tv ]) {
        NSLog(@"Error loading Nib for document!");
        return;
        //        return 0;
    } else {
        NSLog(@"Loaded");
        NSLog(@"Returning %p", tv);
        return;
        //        return dv;
    }

    
    
}