/*
 *  CARecord.h
 *  xwaxMac
 *
 *  Created by Tom Blench on 02/10/2009.
 *  Copyright 2009 Zen_Platypus. All rights reserved.
 *
 */

#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>

class CARecord
{
public:
    OSStatus ConfigureOutoutFile(const FSRef inParentDirectory, const CFStringRef inFileName, AudioStreamBasicDescription *inABSD);
};
