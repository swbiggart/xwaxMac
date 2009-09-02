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
	signed short *loadAudioFile(const char* fileName, size_t *outbytes);
#ifdef __cplusplus
}
#endif