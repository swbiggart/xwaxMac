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

int loadAudioFile(const char* fileName, struct track_t *tr)
{
    // ExtAudioFile
    OSStatus status;
    UInt32 dataSize;
    
    AUBufferList m_auBufferList; //from file
    
    FSRef fileRef;
    FSPathMakeRef((const UInt8*)fileName, &fileRef, false);
    
    if (ExtAudioFileOpen == NULL)
        return -1;
    ExtAudioFileRef audioFileRef = NULL;
    
    status = ExtAudioFileOpen(&fileRef, &audioFileRef);
    if (status != noErr)
        return -1;
    
    SInt64 audioFileNumFrames = 0;
    dataSize = sizeof(audioFileNumFrames);
    status = ExtAudioFileGetProperty(audioFileRef, kExtAudioFileProperty_FileLengthFrames, &dataSize, &audioFileNumFrames);
    if (status != noErr)
        return -1;
    
    AudioStreamBasicDescription audioFileStreamFormat;
    dataSize = sizeof(audioFileStreamFormat);
    status = ExtAudioFileGetProperty(audioFileRef, kExtAudioFileProperty_FileDataFormat, &dataSize, &audioFileStreamFormat);
    if (status != noErr)
        return -1;
    
    // Setup stream format that we want - unsigned int interleaved suitable for xwax
    AudioStreamBasicDescription des;    
    FillOutASBDForLPCM(des, audioFileStreamFormat.mSampleRate, audioFileStreamFormat.mChannelsPerFrame, 16, 16, false, false, false);
    CAStreamBasicDescription clientStreamFormat(des);
    clientStreamFormat.Print(stdout);

    status = ExtAudioFileSetProperty(audioFileRef, kExtAudioFileProperty_ClientDataFormat, sizeof(clientStreamFormat), &clientStreamFormat);
    if (status != noErr) {
        fprintf(stderr,"Problem with conversion %ld!\n", status);
        return -1;
    }    
    
    // Allocate buffer and load file
    m_auBufferList.Allocate(clientStreamFormat, (UInt32)audioFileNumFrames);
    AudioBufferList & abl = m_auBufferList.PrepareBuffer(clientStreamFormat, (UInt32)audioFileNumFrames);
    UInt32 audioFileNumFrames_temp = (UInt32)audioFileNumFrames;
    status = ExtAudioFileRead(audioFileRef, &audioFileNumFrames_temp, &abl);
    
    if (status != noErr) {
        m_auBufferList.Deallocate();
        return -1;
    }
    if (audioFileNumFrames_temp != (UInt32)audioFileNumFrames) {
        // This doesn't appear to be an error
        fprintf(stderr, ":  audio data size mismatch!\nsize requested: %lu, size read: %lu\n\n", (UInt32)audioFileNumFrames, audioFileNumFrames_temp);
    }
    
    status = ExtAudioFileDispose(audioFileRef);

    tr->bufsiz = (size_t)(&m_auBufferList.GetBufferList())->mBuffers[0].mDataByteSize;
    tr->buf = (short*)(&m_auBufferList.GetBufferList())->mBuffers[0].mData;
    // copy data into blocks used by track in xwax
    read_from_buffer(tr);
    
    // AU buffers get dealloced here by deconstructor for m_auBufferList
    return 0;
}
