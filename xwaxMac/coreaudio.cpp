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

int coreaudio_init(struct device_t *dv, const char *inDevice, const int inChanL, const int inChanR, 
										const char *outDevice, const int outChanL, const int outChanR, int latency)
{
	AudioDeviceList *inputs = new AudioDeviceList(true);
	AudioDeviceList *outputs = new AudioDeviceList(false);
	
	int inId=-1, outId=-1;
	
	std::string inputsStr, outputsStr;
	
	// Inputs
	{
		AudioDeviceList::DeviceList &thelist = inputs->GetList();
		int index = 0;
		for (AudioDeviceList::DeviceList::iterator i = thelist.begin(); i != thelist.end(); ++i, ++index) {
			inputsStr += "\"";
			inputsStr += (*i).mName;
			inputsStr += "\", "; 
			if (inId == -1 && !strcmp((*i).mName, inDevice))
			{
				inId = (*i).mID;
			}
		}
	}
	// Outputs
	{
		AudioDeviceList::DeviceList &thelist = outputs->GetList();
		int index = 0;
		for (AudioDeviceList::DeviceList::iterator i = thelist.begin(); i != thelist.end(); ++i, ++index) {
			outputsStr += "\"";
			outputsStr += (*i).mName;
			outputsStr += "\", "; 
			if (outId == -1 && !strcmp((*i).mName, outDevice))
			{
				outId = (*i).mID;
				break;
			}
		}
	}
	
	if (inId == -1 || outId == -1)
	{
		if (inId == -1)
		{
			fprintf(stderr, "Error starting up, couldn't find input device \"%s\"\n", inDevice);
			std::cerr << "Possible input devices: " << inputsStr << std::endl;
		}
		if (outId == -1)
		{
			fprintf(stderr, "Error starting up, couldn't find output device \"%s\"\n", outDevice);
			std::cerr << "Possible output devices: " << outputsStr << std::endl;
		}
		return -1;
	}
	printf("Starting up a deck with devices %s (%d) %s (%d) ins %d %d outs %d %d\n", inDevice, inId, outDevice, outId, inChanL, inChanR, outChanL, outChanR);
	pt[decks] = new CAPlayThroughHost(inId, outId, inChanL, inChanR, outChanL, outChanR, dv, latency);
	dv->type = &coreaudio_type;
	assert(decks < MAX_DECKS);
    device[decks] = dv;
    decks++;
	
	return 0;
}

int coreaudio_init_alt(struct device_t *dv, const char *inName, const char *outName)
{



	// Split out the channel numbers from the name
	int inChanL, inChanR;
	char inDevice[512];
	if (sscanf(inName,"%[^:]:%d:%d",inDevice, &inChanL, &inChanR) != 3)
	{
		fprintf(stderr, "Error starting up, couldn't parse input device name \"%s\"\n", inName);
		return -1;
	}

	int outChanL, outChanR;
	char outDevice[512];
	if (sscanf(outName,"%[^:]:%d:%d",outDevice, &outChanL, &outChanR) != 3)
	{
		fprintf(stderr, "Error starting up, couldn't parse output device name \"%s\"\n", outName);
		return -1;
	}
	return coreaudio_init(dv, inName, inChanL, inChanR, outName, outChanL, outChanR, 512);
}