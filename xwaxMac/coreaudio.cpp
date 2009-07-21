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


CAPlayThroughHost *pt;
struct device_t *device[MAX_DECKS];

static int decks = 0,
started = 0;


static unsigned int sample_rate(struct device_t *dv)
{
	printf("coreaudio sample_rate\n");
	return pt->GetSampleRate(); // FIXME
}

static int start(struct device_t *dv)
{
	
	printf("coreaudio start\n");
	pt->Start();
	
}
static int stop(struct device_t *dv)
{
	printf("coreaudio stop\n");
}
static void clear(struct device_t *dv)
{
	printf("coreaudio clear");
}

static struct device_type_t coreaudio_type = {
NULL,
NULL, /* done via callbacks */
sample_rate,
start,
stop,
clear
};


int coreaudio_init(struct device_t *dv, const char *inName, const char *outName)
{
	AudioDeviceList *inputs = new AudioDeviceList(true);
	AudioDeviceList *outputs = new AudioDeviceList(false);
	
	int inId=-1, outId=-1;

	
	//TODO - take channel numbers in addition to device name and map them to decks - or maybe channel numbers are automatic???
	//Manage a number of playthroughs depending on number of unique input/output combinations
	//ie if started with -c in out -c in out then let jack create the decks for both of these
	//but behind the scenes manage one instance of playthrough, 1st one processing channels 1,2 and 2nd 3,4
	std::string inputsStr, outputsStr;
	
//	printf("Inputs:\n");
	{
		AudioDeviceList::DeviceList &thelist = inputs->GetList();
		int index = 0;
		for (AudioDeviceList::DeviceList::iterator i = thelist.begin(); i != thelist.end(); ++i, ++index) {
			inputsStr += "\"";
			inputsStr += (*i).mName;
			inputsStr += "\", "; 
//			printf ("%s %lu\n",(*i).mName, (*i).mID);
			if (inId == -1 && !strcmp((*i).mName, inName))
			{
				inId = (*i).mID;
			}
		}
	}
//	printf("Outputs:\n");
	{
		AudioDeviceList::DeviceList &thelist = outputs->GetList();
		int index = 0;
		for (AudioDeviceList::DeviceList::iterator i = thelist.begin(); i != thelist.end(); ++i, ++index) {
			outputsStr += "\"";
			outputsStr += (*i).mName;
			outputsStr += "\", "; 
//			printf ("%s %lu\n",(*i).mName, (*i).mID);
			if (outId == -1 && !strcmp((*i).mName, outName))
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
			fprintf(stderr, "Error starting up, couldn't find input device \"%s\"\n", inName);
			std::cerr << "Possible input devices: " << inputsStr << std::endl;
		}
		if (outId == -1)
		{
			fprintf(stderr, "Error starting up, couldn't find output device \"%s\"\n", outName);
			std::cerr << "Possible output devices: " << outputsStr << std::endl;
		}
		return -1;
	}
	
	pt = new CAPlayThroughHost(inId, outId, dv);// b-i mic, b-i out
//	pt = new CAPlayThroughHost(265, 267, dv);// b-i mic, b-i out
//	pt = new CAPlayThroughHost(270, 267, dv);//jackrouter, b-i out
	dv->type = &coreaudio_type;
	assert(decks < MAX_DECKS);
    device[decks] = dv;
    decks++;
	
	//sleep(600000000);
	return 0;
}