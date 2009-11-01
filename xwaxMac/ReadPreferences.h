/*
 *  ReadPreferences.h
 *  xwaxMac
 *
 *  Created by Thomas Blench on 16/09/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef ReadPreferences_h
#define ReadPreferences_h

struct iopair
{
    int inDeviceId;
    char inDeviceName[64];
    int inDeviceChanL;
    int inDeviceChanR;
    int outDeviceId;
    char outDeviceName[64];
    int outDeviceChanL;
    int outDeviceChanR;
    int enabled;
};

struct prefs
{
    struct iopair *ios;
    int latency;
    int nDecks;
    char timecode[64];
    int recordDeviceId;
    char recordDeviceName[64];
    int recordDeviceChanL;
    int recordDeviceChanR;
    int recordBitrate;
    char recordFormat[64];
    char recordPath[1024];
    int recordEnabled; // 0 or 1
};

#ifdef __cplusplus
extern "C" {
#endif
int readPreferences(struct prefs *prefs);
int showPrefsWindow(struct prefs *prefs);
#ifdef __cplusplus
};
#endif

#endif