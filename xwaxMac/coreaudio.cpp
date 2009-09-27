/*
 *  coreaudio.cpp
 *  xwaxMac
 *
 *  Created by Thomas Blench on 16/07/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "coreaudio.h"

#include "AudioDeviceList.h"
#include "CAPlayThrough.h"
#include <string>
#include <iostream>

#define MAX_DECKS 4


CAPlayThroughHost *pt[MAX_DECKS];
struct device_t *device[MAX_DECKS];

static int decks = 0;


static unsigned int sample_rate(struct device_t *dv)
{
    printf("coreaudio sample_rate\n");
    return pt[0]->GetSampleRate(); // FIXME
}

static int start(struct device_t *dv)
{
    
    printf("coreaudio start\n");
    int i;
    for(i=0;i<decks;i++)
    {
    pt[i]->Start();
    }
    return 0;
}
//FIXME
static int stop(struct device_t *dv)
{
    printf("coreaudio stop\n");
    int i;
    for(i=0;i<decks;i++)
    {
        pt[i]->Stop();
    }
    return 0;
}
//FIXME
static void clear(struct device_t *dv)
{
    printf("coreaudio clear");
    return;
}

static struct device_type_t coreaudio_type = {
NULL,
NULL, /* done via callbacks */
sample_rate,
start,
stop,
clear
};

int coreaudio_id_for_device(char *deviceName, int isInput)
{
    AudioDeviceList *devices = new AudioDeviceList(isInput);
    int id=-1;
    {
        AudioDeviceList::DeviceList &thelist = devices->GetList();
        for (AudioDeviceList::DeviceList::iterator i = thelist.begin(); i != thelist.end(); ++i) {
            if (id == -1 && !strcmp((*i).mName, deviceName)) {
                id = (*i).mID;
            }
        }
    }
    delete (devices);
    return id;
}

int coreaudio_init(struct device_t *dv, const int inId, const int inChanL, const int inChanR, 
                                        const int outId, const int outChanL, const int outChanR, int latency)
{
    pt[decks] = new CAPlayThroughHost(inId, outId, inChanL, inChanR, outChanL, outChanR, dv, latency);
    dv->type = &coreaudio_type;
    assert(decks < MAX_DECKS);
    device[decks] = dv;
    decks++;
    
    return 0;
}