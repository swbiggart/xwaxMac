/*
 *  TrieMatcher.h
 *  xwaxMac
 *
 *  Created by Tom Blench on 16/10/2009.
 *  Copyright 2009 Zen_Platypus. All rights reserved.
 *
 */

#include <list>
#include <set>
extern "C" {
#include "library.h"
#include "listing.h"   

}

struct recordcompare
{
    bool operator()(const struct record_t* s1, const struct record_t* s2) const
    {
        int artistcomp = strcmp(s1->artist, s2->artist);
        if (artistcomp != 0)
        {
            return artistcomp < 0;
        }        
        bool titlecomp = strcmp(s1->title, s2->title) < 0;
        return titlecomp;        
    }
};

class Result
    {
    public:
        std::set<struct record_t*, recordcompare> data;
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