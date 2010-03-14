//
//  ListingDataSource.h
//
//  Created by Tom Blench on 30/01/2010.
//  Copyright 2010 Man of Leisure. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface ListingDataSource : NSObject  {
    NSMutableArray *items;
}

@end

@interface ListingTableView : NSWindow  {
	IBOutlet NSSearchField	*searchField;
}
- (void)controlTextDidChange:(NSNotification *)obj;
@end