/*
 *  TrieMatcher.cpp
 *  xwaxMac
 *
 *  Created by Tom Blench on 16/10/2009.
 *  Copyright 2009 Zen_Platypus. All rights reserved.
 *
 */

#include "TrieMatcher.h"
#include "TrieMatcherC.h"
#include <algorithm>

int nodes;
int results;
int resultnodes;
Matcher::Matcher() :
root('\0')
{
    
}


void Matcher::Add(char *str, struct record_t *data)
{

    // Insert to main index
    this->index.data.insert(data);

    TrieNode *t = &root;
    do
    {
        TrieNode *newT;
        if (newT = t->childWithTag(tolower(*str)))
        {
            t = newT;
        }
        else
        {
            newT = new TrieNode(tolower(*str));
            nodes++;
            t->children.push_back(newT);
            t = newT;
        }
        if (!t->data)
        {
            t->data = new Result();
            resultnodes++;
        }
        // Insert for this Trie path
        t->data->data.insert(data);
        results++;
        
    } while(*++str);
}

Result *Matcher::Lookup(char *str)
{
    TrieNode *t = &root;
    do
    {
        TrieNode *newT;
        if (newT = t->childWithTag(*str))
        {
            t = newT;
        }
        else
        {
            return 0;
        }
    } while(*++str);
    return t->data;
}

Result *Matcher::GetIndex()
{
    return &this->index;
}

TrieNode::TrieNode(char t)
{
    tag = t;
    data = 0;
}
TrieNode *TrieNode::childWithTag(char t)
{
    std::list<TrieNode*>::iterator it;
    for (it = children.begin(); it != children.end(); it++) 
    {
        TrieNode *child = *it;
        if (child->tag == t)
        {
            return child;
        }
    }
    return 0;
}

// C Functions
// Global matcher
Matcher m;

void TrieMatcherAdd(char *key, struct record_t *val)
{
    // Add this value keyed by string and all substrings
    char *keyp = key;
    do
    {
        m.Add(keyp,val);
    } while (*++keyp);
       
}

void TrieMatcherLookup(char *key, struct listing_t *l)
{
    Result *r = m.Lookup(key);
    
    if (r)
    {
        l->entries = r->data.size();
        l->size    = l->entries;
        l->record = (struct record_t**)realloc(l->record,sizeof(struct record_t*)*l->entries);
        std::set<struct record_t*>::iterator it;
        int i;
        for (i=0,it = r->data.begin(); it != r->data.end(); i++,it++)
        {
            l->record[i] = *it;
        }
    }    
}
void IndexLookup(char *key, struct listing_t *l)
{
    Result *r = m.GetIndex();
    
    if (r)
    {
        l->entries = r->data.size();
        l->size    = l->entries;
        l->record = (struct record_t**)realloc(l->record,sizeof(struct record_t*)*l->entries);
        std::set<struct record_t*>::iterator it;
        int i;
        for (i=0,it = r->data.begin(); it != r->data.end(); i++,it++)
        {
            l->record[i] = *it;
        }
    }    
}
