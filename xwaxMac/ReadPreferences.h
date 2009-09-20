/*
 *  ReadPreferences.h
 *  xwaxMac
 *
 *  Created by Thomas Blench on 16/09/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

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
    char *timecode;
};

#ifdef __cplusplus
extern "C" {
#endif
int readPreferences(struct prefs *prefs);
int showPrefsWindow(struct prefs *prefs);
#ifdef __cplusplus
};
#endif