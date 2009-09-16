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
int coreaudio_init_alt(struct device_t *dv, const char *inName, const char *outName);
int coreaudio_init(struct device_t *dv, const char *inName, const int inChanL, const int inChanR, 
				   const char *outName, const int outChanL, const int outChanR, int latency);

#ifdef __cplusplus
}
#endif