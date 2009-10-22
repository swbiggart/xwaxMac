/*
 *  TrieMatcherC.h
 *  xwaxMac
 *
 *  Created by Tom Blench on 16/10/2009.
 *  Copyright 2009 Zen_Platypus. All rights reserved.
 *
 */


#ifdef __cplusplus
extern "C" {
#endif
#include "library.h"
void TrieMatcherAdd(char *key, struct record_t *val);
void TrieMatcherLookup(char *key, struct listing_t *l);
void IndexLookup(char *key, struct listing_t *l);
#ifdef __cplusplus
};
#endif