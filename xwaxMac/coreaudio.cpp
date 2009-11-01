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
#include "CARecord.h"
#include <string>
#include <iostream>

#define MAX_DECKS 4

//FIXME cleanup globals
struct device_t *device[MAX_DECKS];

static int decks = 0;
CARecord recorder;

static unsigned int sample_rate(struct device_t *dv)
{
    printf("coreaudio sample_rate\n");
    return ((CAPlayThroughHost*)device[0]->local)->GetSampleRate(); // FIXME
}

static int start(struct device_t *dv)
{
    printf("coreaudio start\n");
    ((CAPlayThroughHost*)dv->local)->Start();
    return 0;
}

static int stop(struct device_t *dv)
{
    printf("coreaudio stop\n");
    ((CAPlayThroughHost*)dv->local)->Stop();
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
    dv->local = new CAPlayThroughHost(inId, outId, inChanL, inChanR, outChanL, outChanR, dv, latency);
    dv->type = &coreaudio_type;
    assert(decks < MAX_DECKS);
    device[decks] = dv;
    decks++;    
    return 0;
}

int coreaudio_setup_record(struct prefs *p)
{
    if (p->recordEnabled)
    {
        recorder.ConfigureDefault(p);
        recorder.Start();
    }
    return 0;
}