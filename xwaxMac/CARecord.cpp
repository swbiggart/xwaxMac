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
#include <algorithm>

CARecord::CARecord()
{
	fInputDeviceID = 0;
	fAudioChannels = fAudioSamples = 0;
}

CARecord::~CARecord()
{
	// Stop pulling audio data
	Stop();
	
	// Dispose our audio file reference
	// Also responsible for flushing async data to disk
	ExtAudioFileDispose(fOutputAudioFile);
}

// Convenience function to dispose of our audio buffers
void CARecord::DestroyAudioBufferList(AudioBufferList* list)
{
	UInt32						i;
	
	if(list) {
		for(i = 0; i < list->mNumberBuffers; i++) {
			if(list->mBuffers[i].mData)
                free(list->mBuffers[i].mData);
		}
		free(list);
	}
}

// Convenience function to allocate our audio buffers
AudioBufferList *CARecord::AllocateAudioBufferList(UInt32 numChannels, UInt32 size)
{
	AudioBufferList*			list;
	UInt32						i;
	
	list = (AudioBufferList*)calloc(1, sizeof(AudioBufferList) + numChannels * sizeof(AudioBuffer));
	if(list == NULL)
        return NULL;
	
	list->mNumberBuffers = numChannels;
	for(i = 0; i < numChannels; ++i) {
		list->mBuffers[i].mNumberChannels = 1;
		list->mBuffers[i].mDataByteSize = size;
		list->mBuffers[i].mData = malloc(size);
		if(list->mBuffers[i].mData == NULL) {
			DestroyAudioBufferList(list);
			return NULL;
		}
	}
	return list;
}

OSStatus CARecord::AudioInputProc(void* inRefCon, AudioUnitRenderActionFlags* ioActionFlags, const AudioTimeStamp* inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList* ioData)
{
	CARecord *afr = (CARecord*)inRefCon;
	OSStatus	err = noErr;
    
	// Render into audio buffer
	err = AudioUnitRender(afr->fAudioUnit, ioActionFlags, inTimeStamp, inBusNumber, inNumberFrames, afr->fAudioBuffer);
	if(err)
		fprintf(stderr, "AudioUnitRender() failed with error %i\n", err);
	
	// Write to file, ExtAudioFile auto-magicly handles conversion/encoding
	// NOTE: Async writes may not be flushed to disk until a the file
	// reference is disposed using ExtAudioFileDispose
	err = ExtAudioFileWriteAsync(afr->fOutputAudioFile, inNumberFrames, afr->fAudioBuffer);
	if(err != noErr)
	{
		char formatID[5];
		*(UInt32 *)formatID = CFSwapInt32HostToBig(err);
		formatID[4] = '\0';
		fprintf(stderr, "ExtAudioFileWrite FAILED! %d '%-4.4s'\n",err, formatID);
		return err;
	}
    
	return err;
}

OSStatus CARecord::ConfigureOutputFile(const FSRef inParentDirectory, const CFStringRef inFileName, AudioStreamBasicDescription *inASBD)
{
	OSStatus err = noErr;
	AudioConverterRef conv = NULL;
    
	// Create new file according to format type
    if (inASBD->mFormatID == kAudioFormatMPEG4AAC)
    {
        err = ExtAudioFileCreateNew(&inParentDirectory, inFileName, kAudioFileM4AType, inASBD, NULL, &fOutputAudioFile);
    } 
    else if (inASBD->mFormatID == kAudioFormatLinearPCM)
    {
        err = ExtAudioFileCreateNew(&inParentDirectory, inFileName, kAudioFileAIFFType, inASBD, NULL, &fOutputAudioFile);
    }
    else if (inASBD->mFormatID == kAudioFormatAppleLossless)
    {
        err = ExtAudioFileCreateNew(&inParentDirectory, inFileName, kAudioFileM4AType, inASBD, NULL, &fOutputAudioFile);
    }

	if(err != noErr)
	{
		char formatID[5];
		*(UInt32 *)formatID = CFSwapInt32HostToBig(err);
		formatID[4] = '\0';
		fprintf(stderr, "ExtAudioFileCreateNew FAILED! %d '%-4.4s'\n",err, formatID);
		return err;
	}
    
	// Inform the file what format the data is we're going to give it, should be pcm
	// You must set this in order to encode or decode a non-PCM file data format.
	err = ExtAudioFileSetProperty(fOutputAudioFile, kExtAudioFileProperty_ClientDataFormat, sizeof(AudioStreamBasicDescription), &fOutputFormat);
	if(err != noErr)
	{
		char formatID[5];
		*(UInt32 *)formatID = CFSwapInt32HostToBig(err);
		formatID[4] = '\0';
		fprintf(stderr, "ExtAudioFileSetProperty FAILED! '%-4.4s'\n", formatID);
		return err;
	}
    
    // Get the underlying AudioConverterRef
    UInt32 size = sizeof(AudioConverterRef);
    err = ExtAudioFileGetProperty(fOutputAudioFile, kExtAudioFileProperty_AudioConverter, &size, &conv);
    
    if(!conv)
    {
        fprintf(stderr,"Warning: Couldn't get conv\n");
    }
    
    // Setup input channel map
    {
        SInt32 *channelMap = NULL;
        UInt32 size = sizeof(SInt32)*2;
        channelMap = (SInt32*)malloc(size);
        // Map desired input channels (0-based) from device channels (1-based)
        channelMap[0] = prefs->recordDeviceChanL-1;
        channelMap[1] = prefs->recordDeviceChanR-1;
        err = AudioConverterSetProperty(conv, kAudioConverterChannelMap, size, channelMap);
        if(err != noErr)
        {
            char formatID[5];
            *(UInt32 *)formatID = CFSwapInt32HostToBig(err);
            formatID[4] = '\0';
            fprintf(stderr, "Setting channel map FAILED! '%-4.4s'\n", formatID);
            return err;
        }
        free(channelMap);
    }
    
    /*
	// If we're recording from a mono source, setup a simple channel map to split to stereo
	if (fDeviceFormat.mChannelsPerFrame == 1 && fOutputFormat.mChannelsPerFrame == 2)
	{
		if (conv)
		{
			// This should be as large as the number of output channels,
			// each element specifies which input channel's data is routed to that output channel
			SInt32 channelMap[] = { 0, 0 };
			err = AudioConverterSetProperty(conv, kAudioConverterChannelMap, 2*sizeof(SInt32), channelMap);
		}
	}
     */

    // Set the bitrate if we are recording in aac
    if (conv && inASBD->mFormatID == kAudioFormatMPEG4AAC)
    {
        //Set the bitrate
        UInt32 bitrate = prefs->recordBitrate * 1000; // 1000 or 1024, what does iTunes do?
        err = AudioConverterSetProperty(conv, kAudioConverterEncodeBitRate, sizeof(UInt32), &bitrate);
        if(err != noErr)
        {
            char formatID[5];
            *(UInt32 *)formatID = CFSwapInt32HostToBig(err);
            formatID[4] = '\0';
            fprintf(stderr, "Setting bitrate FAILED! '%-4.4s'\n", formatID);
            return err;
        }
    }    

    
	// Initialize async writes thus preparing it for IO
	err = ExtAudioFileWriteAsync(fOutputAudioFile, 0, NULL);
	if(err != noErr)
	{
		char formatID[5];
		*(UInt32 *)formatID = CFSwapInt32HostToBig(err);
		formatID[4] = '\0';
		fprintf(stderr, "ExtAudioFileWriteAsync FAILED! '%-4.4s'\n", formatID);
		return err;
	}
    
	return err;
}

OSStatus CARecord::ConfigureAU()
{
	Component					component;
	ComponentDescription		description;
	OSStatus	err = noErr;
	UInt32	param;
	AURenderCallbackStruct	callback;
    
	// Open the AudioOutputUnit
	// There are several different types of Audio Units.
	// Some audio units serve as Outputs, Mixers, or DSP
	// units. See AUComponent.h for listing
	description.componentType = kAudioUnitType_Output;
	description.componentSubType = kAudioUnitSubType_HALOutput;
	description.componentManufacturer = kAudioUnitManufacturer_Apple;
	description.componentFlags = 0;
	description.componentFlagsMask = 0;
	if(component = FindNextComponent(NULL, &description))
	{
		err = OpenAComponent(component, &fAudioUnit);
		if(err != noErr)
		{
			fAudioUnit = NULL;
			return err;
		}
	}
    
	// Configure the AudioOutputUnit
	// You must enable the Audio Unit (AUHAL) for input and output for the same  device.
	// When using AudioUnitSetProperty the 4th parameter in the method
	// refer to an AudioUnitElement.  When using an AudioOutputUnit
	// for input the element will be '1' and the output element will be '0'.	
	
	// Enable input on the AUHAL
	param = 1;
	err = AudioUnitSetProperty(fAudioUnit, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Input, 1, &param, sizeof(UInt32));
	if(err == noErr)
	{
		// Disable Output on the AUHAL
		param = 0;
		err = AudioUnitSetProperty(fAudioUnit, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Output, 0, &param, sizeof(UInt32));
	}
    
	// Set the current device to the default input unit.
	err = AudioUnitSetProperty(fAudioUnit, kAudioOutputUnitProperty_CurrentDevice, kAudioUnitScope_Global, 0, &fInputDeviceID, sizeof(AudioDeviceID));
	if(err != noErr)
	{
		fprintf(stderr, "failed to set AU input device\n");
		return err;
	}
	
	// Setup render callback
	// This will be called when the AUHAL has input data
	callback.inputProc = CARecord::AudioInputProc; // defined as static in the header file
	callback.inputProcRefCon = this;
	err = AudioUnitSetProperty(fAudioUnit, kAudioOutputUnitProperty_SetInputCallback, kAudioUnitScope_Global, 0, &callback, sizeof(AURenderCallbackStruct));
    
	// get hardware device format
	param = sizeof(AudioStreamBasicDescription);
	err = AudioUnitGetProperty(fAudioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 1, &fDeviceFormat, &param);
	if(err != noErr)
	{
		fprintf(stderr, "failed to get input device ASBD\n");
		return err;
	}
    
	// Twiddle the format to our liking
	fAudioChannels = std::max(fDeviceFormat.mChannelsPerFrame, (UInt32)2);
	fOutputFormat.mChannelsPerFrame = fAudioChannels;
	fOutputFormat.mSampleRate = fDeviceFormat.mSampleRate;
	fOutputFormat.mFormatID = kAudioFormatLinearPCM;
	fOutputFormat.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked | kAudioFormatFlagIsNonInterleaved;
	if (fOutputFormat.mFormatID == kAudioFormatLinearPCM && fAudioChannels == 1)
		fOutputFormat.mFormatFlags &= ~kLinearPCMFormatFlagIsNonInterleaved;
#if __BIG_ENDIAN__
	fOutputFormat.mFormatFlags |= kAudioFormatFlagIsBigEndian;
#endif
	fOutputFormat.mBitsPerChannel = sizeof(Float32) * 8;
	fOutputFormat.mBytesPerFrame = fOutputFormat.mBitsPerChannel / 8;
	fOutputFormat.mFramesPerPacket = 1;
	fOutputFormat.mBytesPerPacket = fOutputFormat.mBytesPerFrame;
    
	// Set the AudioOutputUnit output data format
	err = AudioUnitSetProperty(fAudioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 1, &fOutputFormat, sizeof(AudioStreamBasicDescription));
	if(err != noErr)
	{
		fprintf(stderr, "failed to set input device ASBD\n");
		return err;
	}
    
	// Get the number of frames in the IO buffer(s)
	param = sizeof(UInt32);
	err = AudioUnitGetProperty(fAudioUnit, kAudioDevicePropertyBufferFrameSize, kAudioUnitScope_Global, 0, &fAudioSamples, &param);
	if(err != noErr)
	{
		fprintf(stderr, "failed to get audio sample size\n");
		return err;
	}
    
	// Initialize the AU
	err = AudioUnitInitialize(fAudioUnit);
	if(err != noErr)
	{
		fprintf(stderr, "failed to initialize AU\n");
		return err;
	}
    
	// Allocate our audio buffers
	fAudioBuffer = AllocateAudioBufferList(fOutputFormat.mChannelsPerFrame, fAudioSamples * fOutputFormat.mBytesPerFrame);
	if(fAudioBuffer == NULL)
	{
		fprintf(stderr, "failed to allocate buffers\n");
		return err;
	}
    
	return noErr;
}

// Configure and Initialize our AudioUnits, Audio Files, and Audio Buffers
OSStatus CARecord::Configure(const FSRef inParentDirectory, const CFStringRef inFileName, AudioStreamBasicDescription *inASBD)
{
	OSStatus err = noErr;
	
	err = ConfigureAU();
	if(err == noErr)
		err = ConfigureOutputFile(inParentDirectory, inFileName, inASBD);
	return err;
}

// Configure and Initialize our AudioUnits, Audio Files, and Audio Buffers
// FIXME: a better name would be configurefromprefs?
OSStatus CARecord::ConfigureDefault(struct prefs *p)
{
	OSStatus err = noErr;

	this->prefs = p;
    fInputDeviceID = p->recordDeviceId;
	err = ConfigureAU();
    
    FSRef inParentDirectory;
    FSPathMakeRef((const UInt8*)p->recordPath, &inParentDirectory, NULL);
    
    time_t rawtime;
    struct tm * timeinfo;
    char buf [128];
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    strftime (buf,128,"recording%Y%m%d%H%M%S",timeinfo);
    
    // FIXME hardcoding of sample rate
    AudioStreamBasicDescription	inASBD;
    if (strcmp(prefs->recordFormat,"AAC")==0)
    {
        AudioStreamBasicDescription asbd = {44100.0, kAudioFormatMPEG4AAC, 0, 0, 0, 0, 2, 0, 0};
        inASBD = asbd;
        sprintf(buf,"%s.m4a",buf);
    }
    else if (strcmp(prefs->recordFormat,"AIFF")==0)
    {

        AudioStreamBasicDescription asbd = {44100.0, kAudioFormatLinearPCM, kAudioFormatFlagIsBigEndian | kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked, 4, 1, 4, 2, 16, 0};
        inASBD = asbd;
        sprintf(buf,"%s.aiff",buf);
    }
    else if (strcmp(prefs->recordFormat,"Apple Lossless")==0)
    {
        AudioStreamBasicDescription asbd = {44100.0, kAudioFormatAppleLossless, kAppleLosslessFormatFlag_16BitSourceData, 0, 0, 4, 2, 16, 0};// FIXME - bit depth hardcoded?
        inASBD = asbd;
        sprintf(buf,"%s.m4a",buf);
    }
    
    CFStringRef inFileName = CFStringCreateWithCString(kCFAllocatorDefault, buf, kCFStringEncodingASCII);
    
	if(err == noErr)
		err = ConfigureOutputFile(inParentDirectory, inFileName, &inASBD);
	return err;
}

OSStatus CARecord::Start()
{
	// Start pulling for audio data
	OSStatus err = AudioOutputUnitStart(fAudioUnit);
	if(err != noErr)
	{
		fprintf(stderr, "failed to start AU\n");
		return err;
	}
	
	fprintf(stderr, "Recording started...\n");
	return err;
}

OSStatus CARecord::Stop()
{
	// Stop pulling audio data
	OSStatus err = AudioOutputUnitStop(fAudioUnit);
	if(err != noErr)
	{
		fprintf(stderr, "failed to stop AU\n");
		return err;
	}
	
	fprintf(stderr, "Recording stopped.\n");
	return err;
}
