/*    
 * Based on CAPlayThrough.cpp sample code
 */
#include "CAPlayThrough.h"
#import "AUWindowController.h"
#include "CAComponent.h"
#include "SimpleRingBuffer.h"
#include <algorithm>
#pragma mark -- CAPlayThrough
typedef Float32 AudioSampleType; 


// we define the class here so that is is not accessible from any object aside from CAPlayThroughManager
class CAPlayThrough 
{
public:
    CAPlayThrough(AudioDeviceID input, AudioDeviceID output, int inChanL, int inChanR, int outChanL, int outChanR, struct device_t * xwax_device, int latency);
    ~CAPlayThrough();
    
    OSStatus    Init(AudioDeviceID input, AudioDeviceID output);
    void        Cleanup();
    OSStatus    Start();
    OSStatus    Stop();
    Boolean        IsRunning();
    OSStatus    SetInputDeviceAsCurrent(AudioDeviceID in);
    OSStatus    SetOutputDeviceAsCurrent(AudioDeviceID out);
    
    AudioDeviceID GetInputDeviceID()    { return mInputDevice.mID;    }
    AudioDeviceID GetOutputDeviceID()    { return mOutputDevice.mID; }
    
    

private:
    OSStatus SetupGraph(AudioDeviceID out);
    OSStatus MakeGraph();
    
    OSStatus SetupAUHAL(AudioDeviceID in);
    OSStatus EnableIO();
    OSStatus CallbackSetup();
    OSStatus SetupBuffers();
        
    static OSStatus InputProc(void *inRefCon,
                              AudioUnitRenderActionFlags *ioActionFlags,
                              const AudioTimeStamp *inTimeStamp,
                              UInt32                inBusNumber,
                              UInt32                inNumberFrames,
                              AudioBufferList *        ioData);
    
    static OSStatus OutputProc(void *inRefCon,
                               AudioUnitRenderActionFlags *ioActionFlags,
                               const AudioTimeStamp *inTimeStamp,
                               UInt32                inBusNumber,
                               UInt32                inNumberFrames,
                               AudioBufferList *    ioData);
                                            
    AudioUnit mInputUnit;
    AudioBufferList *mInputBuffer;
    AudioDevice mInputDevice, mOutputDevice;
    //Ringbuffer for piping samples from input to output for "thru" mode
    SimpleRingBuffer *mBufferL, *mBufferR;

    //AudioUnits and Graph
    AUGraph mGraph;

    //Effect on output
    //TODO effect switching and 0..4 effect slots like Logic
    AUNode mEffectsNode;
    AudioUnit mEffectsUnit;    
    
    //Output unit for selected output device
    AUNode mOutputNode;
    AudioUnit mOutputUnit;
        
    CAComponent *mAUList;
    
    int inChanL,  inChanR,  outChanL,  outChanR;//channel map on input and output devices
    bool wasPlayThruLast;
    signed short *pcmData_submit; // for submitting to timecoder
    signed short *pcmData_fetch; // for fetching from player
    int mLatency;
public:
    Float64 mSampleRate; //only valid after setupbuffers()
    struct device_t *xwax_device;

};

#pragma mark ---Public Methods---

#pragma mark ---CAPlayThrough Methods---

//Construct PlayThrough object associated with xwax device
CAPlayThrough::CAPlayThrough(AudioDeviceID input, AudioDeviceID output, int inChanL, int inChanR, int outChanL, int outChanR, struct device_t *xwax_device, int latency):
mBufferL(NULL),
mBufferR(NULL),
xwax_device(xwax_device),
mSampleRate(-1),
inChanL(inChanL),
inChanR(inChanR),
outChanL(outChanL),
outChanR(outChanR),
wasPlayThruLast(false),
pcmData_submit(NULL),
pcmData_fetch(NULL),
mLatency(latency)
{
    OSStatus err = noErr;
    err =Init(input,output);
    if(err) {
        fprintf(stderr,"CAPlayThrough ERROR: Cannot Init CAPlayThrough");
        exit(1);
    }
}

CAPlayThrough::~CAPlayThrough()
{   
    Cleanup();
}

OSStatus CAPlayThrough::Init(AudioDeviceID input, AudioDeviceID output)
{
    OSStatus err = noErr;
    
    //Setup AUHAL for an input device
    err = SetupAUHAL(input);
    checkErr(err);
    
    //Setup Graph containing Default Output Unit
    err = SetupGraph(output);    
    checkErr(err);
    
    err = SetupBuffers();
    checkErr(err);
            
    err = AUGraphInitialize(mGraph); 
    checkErr(err);
    
    //Add latency between the two devices
        
    return err;    
}

void CAPlayThrough::Cleanup()
{
    //clean up
    Stop();
            
    delete mBufferL;
    delete mBufferR;
    mBufferL = 0;
    mBufferR = 0;
    if(mInputBuffer){
        for(UInt32 i = 0; i<mInputBuffer->mNumberBuffers; i++)
            free(mInputBuffer->mBuffers[i].mData);
        free(mInputBuffer);
        mInputBuffer = 0;
    }
    
    AudioUnitUninitialize(mInputUnit);
    AUGraphClose(mGraph);
    DisposeAUGraph(mGraph);
}

#pragma mark --- Operation---

OSStatus CAPlayThrough::Start()
{
    OSStatus err = noErr;
    if(!IsRunning()){        
        //Start pulling for audio data
        err = AudioOutputUnitStart(mInputUnit);
        checkErr(err);
        
        err = AUGraphStart(mGraph);
        checkErr(err);
    }
    return err;    
}

OSStatus CAPlayThrough::Stop()
{
    OSStatus err = noErr;
    if(IsRunning()){
        //Stop the AUHAL
        err = AudioOutputUnitStop(mInputUnit);
        err = AUGraphStop(mGraph);
    }
    return err;
}

Boolean CAPlayThrough::IsRunning()
{    
    OSStatus err = noErr;
    UInt32 auhalRunning = 0, size = 0;
    Boolean graphRunning;
    size = sizeof(auhalRunning);
    if(mInputUnit)
    {
        err = AudioUnitGetProperty(mInputUnit,
                                kAudioOutputUnitProperty_IsRunning,
                                kAudioUnitScope_Global,
                                0, // input element
                                &auhalRunning,
                                &size);
    }
    
    if(mGraph)
        err = AUGraphIsRunning(mGraph,&graphRunning);
    
    return (auhalRunning || graphRunning);    
}


OSStatus CAPlayThrough::SetOutputDeviceAsCurrent(AudioDeviceID out)
{
    OSStatus err = noErr;
    
    mOutputDevice.Init(out, false);
    checkErr(err);
    
    //Set the Current Device to the Default Output Unit.
    err = AudioUnitSetProperty(mOutputUnit,
                              kAudioOutputUnitProperty_CurrentDevice, 
                              kAudioUnitScope_Global, 
                              0, 
                              &mOutputDevice.mID, 
                              sizeof(mOutputDevice.mID));
    checkErr(err);

    // Set desired latency, or minimum, whichever greatest
    AudioValueRange range;
    UInt32 size = sizeof(range);
    UInt32 latency;
    err = AudioDeviceGetProperty(out, 0, false,kAudioDevicePropertyBufferFrameSizeRange, &size, &range);
    printf("Latency %lf %lf\n", range.mMinimum, range.mMaximum);
    latency = std::max((UInt32)mLatency, (UInt32)range.mMinimum);
    err = AudioDeviceSetProperty(out, NULL, 0, false, kAudioDevicePropertyBufferFrameSize, sizeof(latency), &latency);
    checkErr(err);
    pcmData_fetch = (signed short*)malloc(latency*4); //bytes - 2bytes per chan, 2 chans

    return err;
}

OSStatus CAPlayThrough::SetInputDeviceAsCurrent(AudioDeviceID in)
{
    OSStatus err = noErr;
    
    mInputDevice.Init(in, true);
    checkErr(err);
    
    //Set the Current Device to the AUHAL.
    //this should be done only after IO has been enabled on the AUHAL.
    err = AudioUnitSetProperty(mInputUnit,
                              kAudioOutputUnitProperty_CurrentDevice, 
                              kAudioUnitScope_Global, 
                              0, 
                              &mInputDevice.mID, 
                              sizeof(mInputDevice.mID));
    checkErr(err);

    // Set desired latency, or minimum, whichever greatest
    AudioValueRange range;
    UInt32 size = sizeof(range);
    UInt32 latency;
    err = AudioDeviceGetProperty(in, 0, false,kAudioDevicePropertyBufferFrameSizeRange, &size, &range);
    printf("Latency %lf %lf\n", range.mMinimum, range.mMaximum);
    latency = std::max((UInt32)mLatency, (UInt32)range.mMinimum);
    err = AudioDeviceSetProperty(in, NULL, 0, false, kAudioDevicePropertyBufferFrameSize, sizeof(latency), &latency);
    checkErr(err);
    pcmData_submit = (signed short*)malloc(latency*4); //bytes - 2bytes per chan, 2 chans
    
    return err;
}

#pragma mark -
#pragma mark --Private methods---
OSStatus CAPlayThrough::SetupGraph(AudioDeviceID out)
{
    OSStatus err = noErr;
    AURenderCallbackStruct output;
    
    //Make a New Graph
    err = NewAUGraph(&mGraph);  
    checkErr(err);

    //Open the Graph, AudioUnits are opened but not initialized    
    err = AUGraphOpen(mGraph);
    checkErr(err);
    
    err = MakeGraph();
    checkErr(err);
        
    err = SetOutputDeviceAsCurrent(out);
    checkErr(err);
    
    //Tell the output unit not to reset timestamps 
    //Otherwise sample rate changes will cause sync loss
    UInt32 startAtZero = 0;
    err = AudioUnitSetProperty(mOutputUnit, 
                              kAudioOutputUnitProperty_StartTimestampsAtZero, 
                              kAudioUnitScope_Global,
                              0,
                              &startAtZero, 
                              sizeof(startAtZero));
    checkErr(err);
    
    output.inputProc = OutputProc;
    output.inputProcRefCon = this;
    
    //Now that we are using (an) effect(s), the callback is on the effects unit and not the output unit
    
    err = AudioUnitSetProperty(mOutputUnit, 
                              kAudioUnitProperty_SetRenderCallback, 
                              kAudioUnitScope_Input,
                              0,
                              &output, 
                              sizeof(output));
    
    checkErr(err);        

    return err;
}
void getComponentsForAUType(OSType inAUType, CAComponent *ioCompBuffer, int count)
{
    CAComponentDescription desc = CAComponentDescription(inAUType);
    CAComponent *last = NULL;
    
    for (int i = 0; i < count; ++i) {
        ioCompBuffer[i] = CAComponent(desc, last);
        last = &(ioCompBuffer[i]);
    }
}
int componentCountForAUType(OSType inAUType)
{
    CAComponentDescription desc = CAComponentDescription(inAUType);
    return desc.Count();
}
OSStatus CAPlayThrough::MakeGraph()
{
    OSStatus err = noErr;
    ComponentDescription outDesc;        

    //Setup Output node and unit
    outDesc.componentType = kAudioUnitType_Output;
    outDesc.componentSubType = kAudioUnitSubType_HALOutput;
    outDesc.componentManufacturer = kAudioUnitManufacturer_Apple;
    outDesc.componentFlags = 0;
    outDesc.componentFlagsMask = 0;
    
#if __MAC_OS_X_VERSION_MIN_REQUIRED < 1050
    err = AUGraphNewNode(mGraph, &outDesc, 0, NULL, &mOutputNode);
#else
    err = AUGraphAddNode(mGraph, &outDesc, &mOutputNode);
#endif
    
    checkErr(err);    
#if __MAC_OS_X_VERSION_MIN_REQUIRED < 1050
    UInt32 dontcare;
    void *dontcare2;
    err = AUGraphGetNodeInfo(mGraph, mOutputNode, &outDesc, &dontcare, &dontcare2, &mOutputUnit);
#else
    err = AUGraphNodeInfo(mGraph, mOutputNode, NULL, &mOutputUnit);   
#endif
    checkErr(err);
    
    return err;
}

OSStatus CAPlayThrough::SetupAUHAL(AudioDeviceID in)
{
    OSStatus err = noErr;
    
    Component comp;
    ComponentDescription desc;
    
    desc.componentType = kAudioUnitType_Output;    
    desc.componentSubType = kAudioUnitSubType_HALOutput;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;
    
    //Finds a component that meets the desc spec's
    comp = FindNextComponent(NULL, &desc);
    if (comp == NULL) exit (-1);
    
    //gains access to the services provided by the component
    OpenAComponent(comp, &mInputUnit);  

    //AUHAL needs to be initialized before anything is done to it
    err = AudioUnitInitialize(mInputUnit);
    checkErr(err);
    
    err = EnableIO();
    checkErr(err);
    
    err= SetInputDeviceAsCurrent(in);
    checkErr(err);
    
    err = CallbackSetup();
    checkErr(err);
    
    //Don't setup buffers until you know what the 
    //input and output device audio streams look like.

    err = AudioUnitInitialize(mInputUnit);

    return err;
}

OSStatus CAPlayThrough::EnableIO()
{    
    OSStatus err = noErr;
    UInt32 enableIO;
    
    //You must enable the Audio Unit (AUHAL) for input and disable output 
    //BEFORE setting the AUHAL's current device.
    
    //Enable input on the AUHAL
    enableIO = 1;
    err =  AudioUnitSetProperty(mInputUnit,
                                kAudioOutputUnitProperty_EnableIO,
                                kAudioUnitScope_Input,
                                1, // input element
                                &enableIO,
                                sizeof(enableIO));
    checkErr(err);
    
    //disable Output on the AUHAL
    enableIO = 0;
    err = AudioUnitSetProperty(mInputUnit,
                              kAudioOutputUnitProperty_EnableIO,
                              kAudioUnitScope_Output,
                              0,   //output element
                              &enableIO,
                              sizeof(enableIO));
    return err;
}

OSStatus CAPlayThrough::CallbackSetup()
{
    OSStatus err = noErr;
    AURenderCallbackStruct input;
    
    input.inputProc = InputProc;
    input.inputProcRefCon = this;
    
    //Setup the input callback. 
    err = AudioUnitSetProperty(mInputUnit, 
                              kAudioOutputUnitProperty_SetInputCallback, 
                              kAudioUnitScope_Global,
                              0,
                              &input, 
                              sizeof(input));
    checkErr(err);
    return err;
}

//FIXME - some of this is redundant/needs to change:
// - xwax doesn't support different sample rates on input/output (although we could do varispeed and fake it for xwax)
//Allocate Audio Buffer List(s) to hold the data from input.
OSStatus CAPlayThrough::SetupBuffers()
{
    OSStatus err = noErr;
    UInt32 bufferSizeFrames,bufferSizeBytes,propsize;
    
    CAStreamBasicDescription asbd,asbd_dev1_in,asbd_dev2_out;            
    Float64 inRate=0, outRate=0;
    
    //Get the size of the IO buffer(s)
    UInt32 propertySize = sizeof(bufferSizeFrames);
    err = AudioUnitGetProperty(mInputUnit, kAudioDevicePropertyBufferFrameSize, kAudioUnitScope_Global, 0, &bufferSizeFrames, &propertySize);
    bufferSizeBytes = bufferSizeFrames * sizeof(Float32);
        
    //Get the Stream Format (Output client side)
    propertySize = sizeof(asbd_dev1_in);
    err = AudioUnitGetProperty(mInputUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 1, &asbd_dev1_in, &propertySize);
    
    //Get the Stream Format (client side)
    propertySize = sizeof(asbd);
    err = AudioUnitGetProperty(mInputUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 1, &asbd, &propertySize);        

    //Get the Stream Format (Output client side)
    propertySize = sizeof(asbd_dev2_out);
    err = AudioUnitGetProperty(mOutputUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &asbd_dev2_out, &propertySize);
    
    //Set the format of all the AUs to the input/output devices channel count
    //For a simple case, you want to set this to the lower of count of the channels
    //in the input device vs output device
    asbd.mChannelsPerFrame =2;
    printf("Info: Input Device channel count=%ld\t Input Device channel count=%ld\n",asbd_dev1_in.mChannelsPerFrame,asbd_dev2_out.mChannelsPerFrame);    
    printf("Info: CAPlayThrough will use %ld channels\n",asbd.mChannelsPerFrame);    
    
    // We must get the sample rate of the input device and set it to the stream format of AUHAL
    propertySize = sizeof(Float64);
    AudioDeviceGetProperty(mInputDevice.mID, 0, 1, kAudioDevicePropertyNominalSampleRate, &propertySize, &inRate);
    asbd.mSampleRate =inRate;
    propertySize = sizeof(asbd);
    
    //Set the new formats to the AUs...
    err = AudioUnitSetProperty(mInputUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 1, &asbd, propertySize);
    checkErr(err);    
    
    //Set the correct sample rate for the output device, but keep the channel count the same
    propertySize = sizeof(Float64);
    AudioDeviceGetProperty(mOutputDevice.mID, 0, 0, kAudioDevicePropertyNominalSampleRate, &propertySize, &outRate);
    asbd.mSampleRate =outRate;
    propertySize = sizeof(asbd);
    err = AudioUnitSetProperty(mOutputUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &asbd, propertySize);
    checkErr(err);

    if (inRate != outRate)
    {
        fprintf(stderr, "Input rate (%f) and output rate (%f) must be the same!\n", inRate, outRate);
        return -1;
    }
    mSampleRate = inRate;
    
    // Setup input channel map
    {
        SInt32 *channelMap = NULL;
        UInt32 size = sizeof(SInt32)*2;
        channelMap = (SInt32*)malloc(size);
        // Map desired input channels (0-based) from device channels (1-based)
        channelMap[0] = inChanL-1;
        channelMap[1] = outChanR-1;
        err = AudioUnitSetProperty(mInputUnit, kAudioOutputUnitProperty_ChannelMap, kAudioUnitScope_Output, 1, channelMap, size);
        checkErr(err);
        free(channelMap);
    }
    
    // Setup output channel map    
    {
        SInt32 *channelMap = NULL;
        UInt32 size = sizeof(SInt32)*asbd_dev2_out.mChannelsPerFrame;
        channelMap = (SInt32*)malloc(size);
        for (UInt32 i=0;i<asbd_dev2_out.mChannelsPerFrame;i++)
        {
            channelMap[i]=-1;
        }
        // Map desired output device channels (1-based) to first and second output of graph (0-based)
        channelMap[outChanL-1] = 0;
        channelMap[outChanR-1] = 1;
        err = AudioUnitSetProperty(mOutputUnit, kAudioOutputUnitProperty_ChannelMap, kAudioUnitScope_Output, 0, channelMap, size);
        checkErr(err);
        free(channelMap);
    }
    
    //calculate number of buffers from channels
    propsize = offsetof(AudioBufferList, mBuffers[0]) + (sizeof(AudioBuffer) *asbd.mChannelsPerFrame);

    //malloc buffer lists
    mInputBuffer = (AudioBufferList *)malloc(propsize);
    mInputBuffer->mNumberBuffers = asbd.mChannelsPerFrame;
    
    //pre-malloc buffers for AudioBufferLists
    for(UInt32 i =0; i< mInputBuffer->mNumberBuffers ; i++) {
        mInputBuffer->mBuffers[i].mNumberChannels = 1;
        mInputBuffer->mBuffers[i].mDataByteSize = bufferSizeBytes;
        mInputBuffer->mBuffers[i].mData = malloc(bufferSizeBytes);
    }
    
    mBufferL = new SimpleRingBuffer();    
    mBufferR = new SimpleRingBuffer();    
    
    return err;
}

/* Stereo interleave */
#define SCALE 32768
static void interleave(signed short *buf, AudioBufferList *cabuf,
                       register UInt32 nframes, int inChanL, int inChanR)
{
    register AudioSampleType *l,*r;

    if (cabuf->mNumberBuffers != 2)
    {
        fprintf(stderr, "Interleave: problem with number of buffers channels %d %d, buffers %lu\n", inChanL, inChanR, cabuf->mNumberBuffers);
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

static void uninterleave(AudioBufferList *cabuf, signed short *buf, 
                       register UInt32 nframes, int outChanL, int outChanR)
{
    register AudioSampleType *l,*r;

    if (cabuf->mNumberBuffers != 2)
    {
        fprintf(stderr, "Uninterleave: problem with number of buffers channels %d %d, buffers %lu\n", outChanL, outChanR, cabuf->mNumberBuffers);
        abort();
    }
    l = (AudioSampleType*)cabuf->mBuffers[0].mData;
    r = (AudioSampleType*)cabuf->mBuffers[1].mData;

    
    while(nframes--) {
        *l = (AudioSampleType)(* buf   ) / SCALE;
        l++;
        *r = (AudioSampleType)(*(buf+1)) / SCALE;
        r++;
        buf+=2;
    }
}

#pragma mark -
#pragma mark -- IO Procs --
OSStatus CAPlayThrough::InputProc(void *inRefCon,
                                    AudioUnitRenderActionFlags *ioActionFlags,
                                    const AudioTimeStamp *inTimeStamp,
                                    UInt32 inBusNumber,
                                    UInt32 inNumberFrames,
                                    AudioBufferList * ioData)
{
    
    OSStatus err = noErr;
    CAPlayThrough *This = (CAPlayThrough *)inRefCon;
    bool playThru = This->xwax_device->player->passthrough;
    
    //Get the new audio data
    err = AudioUnitRender(This->mInputUnit,
                         ioActionFlags,
                         inTimeStamp, 
                         inBusNumber,     
                         inNumberFrames, //# of frames requested
                         This->mInputBuffer);// Audio Buffer List to hold data
    checkErr(err);
    if(!err)
    {
        if (!playThru)
        {        
            if(This->xwax_device->timecoder)
            {
                interleave(This->pcmData_submit, This->mInputBuffer, inNumberFrames, This->inChanL, This->inChanR);
                timecoder_submit(This->xwax_device->timecoder, This->pcmData_submit, (size_t)inNumberFrames);
            }
        }
        else 
        {
            if (This->wasPlayThruLast == false)
            {
                fprintf(stderr, "Initing buffer\n");
                This->mBufferL->reinit();
                This->mBufferR->reinit();
            }
            err = This->mBufferL->store(inNumberFrames, (AudioSampleType*)This->mInputBuffer->mBuffers[0].mData);
            if (err != 0)
            {
                fprintf(stderr, "Problem storing to buffer %ld\n", err);
            }
            err = This->mBufferR->store(inNumberFrames, (AudioSampleType*)This->mInputBuffer->mBuffers[1].mData);
            if (err != 0)
            {
                fprintf(stderr, "Problem storing to buffer %ld\n", err);
            }
            
        }
    }
    This->wasPlayThruLast = playThru;
    return err;
}

inline void MakeBufferSilent (AudioBufferList * ioData)
{
    for(UInt32 i=0; i<ioData->mNumberBuffers;i++)
        memset(ioData->mBuffers[i].mData, 0, ioData->mBuffers[i].mDataByteSize);    
}

OSStatus CAPlayThrough::OutputProc(void *inRefCon,
                                     AudioUnitRenderActionFlags *ioActionFlags,
                                     const AudioTimeStamp *TimeStamp,
                                     UInt32 inBusNumber,
                                     UInt32 inNumberFrames,
                                     AudioBufferList * ioData)
{
    OSStatus err = noErr;

    CAPlayThrough *This = (CAPlayThrough *)inRefCon;
    bool playThru = This->xwax_device->player->passthrough;

    if (!playThru)
    {
        player_collect(This->xwax_device->player, This->pcmData_fetch, inNumberFrames, This->mSampleRate);//FIXME sample rate
        uninterleave(ioData, This->pcmData_fetch, inNumberFrames, This->outChanL, This->outChanR);
    }
    else
    {
        err = This->mBufferL->fetch(inNumberFrames, (AudioSampleType*)ioData->mBuffers[0].mData);    
        if(err != 0)
        {
            fprintf(stderr, "Buffer empty");
            MakeBufferSilent (ioData);
            
        }
        err = This->mBufferR->fetch(inNumberFrames, (AudioSampleType*)ioData->mBuffers[1].mData);    
        if(err != 0)
        {
            fprintf(stderr, "Buffer empty");
            MakeBufferSilent (ioData);
        }
    }
    return noErr;
}

#pragma mark -- CAPlayThroughHost Methods --

CAPlayThroughHost::CAPlayThroughHost(AudioDeviceID input, AudioDeviceID output, int inChanL, int inChanR, int outChanL, int outChanR, struct device_t *xwax_device, int latency):
    mPlayThrough(NULL)
{
    CreatePlayThrough(input, output,  inChanL,  inChanR,  outChanL,  outChanR, xwax_device, latency);
}

CAPlayThroughHost::~CAPlayThroughHost()
{
    DeletePlayThrough();
}

void CAPlayThroughHost::CreatePlayThrough(AudioDeviceID input, AudioDeviceID output, int inChanL, int inChanR, int outChanL, int outChanR, struct device_t *xwax_device, int latency)
{
    mPlayThrough = new CAPlayThrough(input, output, inChanL, inChanR, outChanL, outChanR, xwax_device, latency);
}

void CAPlayThroughHost::DeletePlayThrough()
{
    if(mPlayThrough)
    {
        mPlayThrough->Stop();
        delete mPlayThrough;
        mPlayThrough = NULL;
    }
}

OSStatus CAPlayThroughHost::Start()
{
    if (mPlayThrough) return mPlayThrough->Start();
    return noErr;
}

OSStatus CAPlayThroughHost::Stop()
{
    if (mPlayThrough) return mPlayThrough->Stop();
    return noErr;
}

Boolean CAPlayThroughHost::IsRunning()
{
    if (mPlayThrough) return mPlayThrough->IsRunning();
    return noErr;
}

Float64 CAPlayThroughHost::GetSampleRate()
{
    return this->GetPlayThrough()->mSampleRate;
}