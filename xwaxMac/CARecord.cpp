/*
 *  CARecord.cpp
 *  xwaxMac
 *
 *  Created by Tom Blench on 02/10/2009.
 *  Copyright 2009 Zen_Platypus. All rights reserved.
 *
 *  See http://developer.apple.com/mac/library/samplecode/RecordAudioToFile/
 */

#include "CARecord.h"
/*
OSStatus CARecord::ConfigureOutoutFile(const FSRef inParentDirectory, const CFStringRef inFileName, AudioStreamBasicDescription *inABSD)
{
    OSStatus err = noErr;
    ExtAudioFileRef audioFile;
    err = ExtAudioFileCreateNew(&inParentDirectory, inFileName, kAudioFileAIFFType, inABSD, NULL, &audioFile);
    if (err!=noErr)
    {
        printf("Error\n");
        return err;
    }
    err = ExtAudioFileSetProperty(audioFile, kExtAudioFileProperty_ClientDataFormat, sizeof(AudioStreamBasicDescription), &audioFile);
    if (err!=noErr)
    {
        printf("Error\n");
        return err;
    }
    err = ExtAudioFileWriteAsync(audioFile, 0, NULL);
    return err;
}*/