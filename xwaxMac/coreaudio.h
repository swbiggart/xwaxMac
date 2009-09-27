/*
 *  coreaudio.h
 *  xwaxMac
 *
 *  Created by Thomas Blench on 16/07/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */
#ifdef __cplusplus
extern "C"
{
#endif
#include "device.h"    
int coreaudio_init(struct device_t *dv, const int inId, const int inChanL, const int inChanR, 
                   const int outId, const int outChanL, const int outChanR, int latency);
int coreaudio_id_for_device(char *deviceName, int isInput);

#ifdef __cplusplus
}
#endif