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
#include <machine/endian.h>
#include "RecordLogger.h"
#include "device.h" //for getting sample rate

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
    
    fprintf(stderr,"Frames %llu\n", audioFileNumFrames);
    // Get sample rate from 0th deck, all decks guaranteed to have same rate
    unsigned int fs = tr->rig->device[0]->type->sample_rate(tr->rig->device[0]);
    // Adjust number of frames due to resampling (there may be a way of getting the API to do this?)
    audioFileNumFrames *= ((float)fs/(float)audioFileStreamFormat.mSampleRate);
    fprintf(stderr,"Adjusted frames %llu\n", audioFileNumFrames);
    
    // Setup stream format that we want - unsigned int interleaved, 2 channel suitable for xwax

    
    AudioStreamBasicDescription des;
#if BYTE_ORDER == LITTLE_ENDIAN
    fprintf(stderr, "I am little\n");
    FillOutASBDForLPCM(des, fs, 2, 16, 16, false, false, false);
#else
    fprintf(stderr, "I am big\n");
    FillOutASBDForLPCM(des, fs, 2, 16, 16, false, true, false);
#endif
    CAStreamBasicDescription clientStreamFormat(des);
    clientStreamFormat.Print(stdout);

    status = ExtAudioFileSetProperty(audioFileRef, kExtAudioFileProperty_ClientDataFormat, sizeof(clientStreamFormat), &clientStreamFormat);
    if (status != noErr) {
        fprintf(stderr,"Problem with conversion %ld!\n", status);
        return -1;
    }    
    
    // Get underlying converter to setup channel map
    AudioConverterRef conv = NULL;
    UInt32 convSize = sizeof(conv);
    status = ExtAudioFileGetProperty(audioFileRef, kExtAudioFileProperty_AudioConverter, &convSize, &conv);    
    if (status != noErr) {
        fprintf(stderr,"Problem with getting converter %ld!\n", status);
        return -1;
    }    
    if (conv != NULL)
    {
        // Setup channel map in case the source is not stereo
        SInt32 *channelMap = NULL;
        UInt32 size = sizeof(SInt32)*2;
        channelMap = (SInt32*)malloc(size);
        // Map from file to desired channels
        // Mono - map to both channels
        if (audioFileStreamFormat.mChannelsPerFrame == 1)
        {
            channelMap[0] = 0;
            channelMap[1] = 0;
        }
        // Stereo or multichannel - pick first two channels
        else 
        {
            channelMap[0] = 0;
            channelMap[1] = 1;
        }
        status = AudioConverterSetProperty(conv, kAudioConverterChannelMap, size, channelMap);
        if (status != noErr) {
            fprintf(stderr,"Problem with setting channel map %ld!\n", status);
            return -1;
        }    
        free(channelMap);
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
    
    // if we got this far then track has been loaded, notify logging service
    //RecordLogger::GetInstance()->LogRecord(tr->title, tr->artist, tr->path);
    
    // AU buffers get dealloced here by deconstructor for m_auBufferList
    return 0;
}
