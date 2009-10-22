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
    // Don't insert if already there
    std::list<struct record_t*>::iterator result = std::find(index.data.begin(), index.data.end(), data);
    if (result == index.data.end())
    {
        this->index.data.push_back(data);
    }
    TrieNode *t = &root;
    do
    {
        TrieNode *newT;
        if (newT = t->childWithTag(tolower(*str)))
        {
            //printf("Found %c\n",*str);
            t = newT;
        }
        else
        {
            //printf("Adding %c\n",*str);
            newT = new TrieNode(tolower(*str));
            nodes++;
            t->children.push_back(newT);
            t = newT;
        }
        // TODO - instead of this, visit all the nodes and sort and unique them as a final pass
        // (What about order?)
        if (!t->data)
        {
            t->data = new Result();
            resultnodes++;
            // Micro-optimisation - don't need to check for unique if we are first
            t->data->data.push_back(data);
        }
        else
        {
            // Don't insert if already there
            std::list<struct record_t*>::iterator result = std::find(t->data->data.begin(), t->data->data.end(), data);
            if (result == t->data->data.end())
            {
                t->data->data.push_back(data);
            }
        }
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
            //printf("Found %c\n",*str);
            t = newT;
        }
        else
        {
            //printf("Didn't find %c\n",*str);
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
        std::list<struct record_t*>::iterator it;
        int i;
        for (i=0,it = r->data.begin(); it != r->data.end(); i++,it++)
        {
            l->record[i] = *it;
            //            printf("%s\n",(*it));
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
        std::list<struct record_t*>::iterator it;
        int i;
        for (i=0,it = r->data.begin(); it != r->data.end(); i++,it++)
        {
            l->record[i] = *it;
            //            printf("%s\n",(*it));
        }
    }    
}
