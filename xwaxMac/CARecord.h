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
#include <Carbon/Carbon.h>
#include "ReadPreferences.h"

class CARecord
{
public:
	CARecord();
	virtual ~CARecord();
    
	AudioBufferList	*AllocateAudioBufferList(UInt32 numChannels, UInt32 size);
	void	DestroyAudioBufferList(AudioBufferList* list);
	OSStatus	ConfigureOutputFile(const FSRef inParentDirectory, const CFStringRef inFileName, AudioStreamBasicDescription *inASBD);
	OSStatus	ConfigureAU();
	OSStatus	Configure(const FSRef inParentDirectory, const CFStringRef inFileName, AudioStreamBasicDescription *inASBD);
    OSStatus    ConfigureDefault(struct prefs *p);
	OSStatus	Start();
	OSStatus	Stop();
    
	AudioBufferList	*fAudioBuffer;
	AudioUnit	fAudioUnit;
	ExtAudioFileRef fOutputAudioFile;
protected:
	static OSStatus AudioInputProc(void* inRefCon, AudioUnitRenderActionFlags* ioActionFlags, const AudioTimeStamp* inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList* ioData);
    
	AudioDeviceID	fInputDeviceID;
	UInt32	fAudioChannels, fAudioSamples;
	AudioStreamBasicDescription	fOutputFormat, fDeviceFormat;
	FSRef fOutputDirectory;
private:
    struct prefs *prefs;
    
};

