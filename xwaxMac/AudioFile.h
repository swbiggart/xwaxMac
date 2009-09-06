/*
 *  AudioFile.h
 *  xwaxMac
 *
 *  Created by Thomas Blench on 30/08/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifdef __cplusplus
extern "C" 
{
#endif
	#include "track.h"
	int loadAudioFile(const char* fileName, struct track_t *tr);
#ifdef __cplusplus
}
#endif