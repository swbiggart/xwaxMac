/*
 *  TrieMatcher.h
 *  xwaxMac
 *
 *  Created by Tom Blench on 16/10/2009.
 *  Copyright 2009 Zen_Platypus. All rights reserved.
 *
 */

#include <list>
extern "C" {
#include "library.h"
#include "listing.h"   

}
class Result
    {
    public:
        std::list<struct record_t*> data;
    };


class TrieNode
    {
    public:
        // Constructors/methods
        TrieNode(char t);
        TrieNode *childWithTag(char t);
    private:
        // Member variables
        char tag;
    public:
        Result *data;
        std::list<TrieNode*> children;        
    };

class Matcher
    {
    public:
        Matcher();
        void Add(char *str, struct record_t *data);
        Result *Lookup(char *str);
        Result *GetIndex();
    private:
        TrieNode root;
        Result index;
        
    };