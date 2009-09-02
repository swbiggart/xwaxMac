/*
 *  AudioFile.cpp
 *  xwaxMac
 *
 *  Created by Thomas Blench on 30/08/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <CoreServices/CoreServices.h>
#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>
#include "CAStreamBasicDescription.h"
#include "AUBuffer.h"
#include "AudioFile.h"

/* Stereo interleave */
#define SCALE 32768
static void interleave(signed short *buf, AudioBufferList *cabuf,
                       register UInt32 nframes)
{
	register AudioSampleType *l,*r;
	if (cabuf->mNumberBuffers != 2)
	{
		fprintf(stderr, "inBig problem %lu\n", cabuf->mNumberBuffers);
		abort();
	}
	l = (AudioSampleType*)cabuf->mBuffers[0].mData;
	r = (AudioSampleType*)cabuf->mBuffers[1].mData;
	
	while(nframes--) {
		*buf = (signed short)(*l * SCALE);
		buf++;
		l++;
		*buf = (signed short)(*r * SCALE);
		buf++;
		r++;
    }
}


signed short *loadAudioFile(const char* fileName, size_t *outbytes)
{
	// ExtAudioFile
	OSStatus status;
	UInt32 dataSize;
	
	bool m_bAudioFileHasBeenLoaded;
	AUBufferList m_auBufferList; //from file
	int m_nNumChannels;	// 1=mono, 2=stereo, 0=yomama
	int m_nNumSamples;	// number of samples per channel
	int m_nSampleRate;
	
	FSRef fileRef;
	FSPathMakeRef((const UInt8*)fileName, &fileRef, false);
	
	if (ExtAudioFileOpen == NULL)
		return NULL;
	ExtAudioFileRef audioFileRef = NULL;
	
	status = ExtAudioFileOpen(&fileRef, &audioFileRef);
	if (status != noErr)
		return NULL;
	
	SInt64 audioFileNumFrames = 0;
	dataSize = sizeof(audioFileNumFrames);
	status = ExtAudioFileGetProperty(audioFileRef, kExtAudioFileProperty_FileLengthFrames, &dataSize, &audioFileNumFrames);
	if (status != noErr)
		return NULL;
	
	AudioStreamBasicDescription audioFileStreamFormat;
	dataSize = sizeof(audioFileStreamFormat);
	status = ExtAudioFileGetProperty(audioFileRef, kExtAudioFileProperty_FileDataFormat, &dataSize, &audioFileStreamFormat);
	if (status != noErr)
		return NULL;
	
	CAStreamBasicDescription clientStreamFormat;
	clientStreamFormat.SetCanonical(audioFileStreamFormat.mChannelsPerFrame, false);
	clientStreamFormat.mSampleRate = audioFileStreamFormat.mSampleRate;
	status = ExtAudioFileSetProperty(audioFileRef, kExtAudioFileProperty_ClientDataFormat, sizeof(clientStreamFormat), &clientStreamFormat);
	if (status != noErr)
		return NULL;
		
	m_bAudioFileHasBeenLoaded = false;	// XXX cuz we're about to possibly re-allocate the audio buffer and invalidate what might already be there
	
	m_auBufferList.Allocate(clientStreamFormat, (UInt32)audioFileNumFrames);
	AudioBufferList & abl = m_auBufferList.PrepareBuffer(clientStreamFormat, (UInt32)audioFileNumFrames);
	UInt32 audioFileNumFrames_temp = (UInt32)audioFileNumFrames;
	status = ExtAudioFileRead(audioFileRef, &audioFileNumFrames_temp, &abl);
	if (status != noErr)
	{
		m_auBufferList.Deallocate();
		return NULL;
	}
	if (audioFileNumFrames_temp != (UInt32)audioFileNumFrames)	// XXX do something?
	{
		// XXX error?
		fprintf(stderr, ":  audio data size mismatch!\nsize requested: %lu, size read: %lu\n\n", (UInt32)audioFileNumFrames, audioFileNumFrames_temp);
	}
	
	status = ExtAudioFileDispose(audioFileRef);
	
	m_nNumChannels = clientStreamFormat.mChannelsPerFrame;
	m_nSampleRate = clientStreamFormat.mSampleRate;
	m_nNumSamples = (int) audioFileNumFrames;
	
	// ready to play
	m_bAudioFileHasBeenLoaded = true;
	
	// interleave
	*outbytes = (size_t)(audioFileStreamFormat.mChannelsPerFrame * audioFileNumFrames * sizeof(signed short));
	signed short *buf = (signed short *)malloc(*outbytes);
	interleave(buf, &m_auBufferList.GetBufferList(), audioFileNumFrames);
	m_auBufferList.Deallocate();
	
	return buf;
}
