/*	
 * Based on CAPlayThrough.cpp sample code
 */
#include "CAPlayThrough.h"
#pragma mark -- CAPlayThrough

// we define the class here so that is is not accessible from any object aside from CAPlayThroughManager
class CAPlayThrough 
{
public:
	CAPlayThrough(AudioDeviceID input, AudioDeviceID output, struct device_t *xwax_device);
	~CAPlayThrough();
	
	OSStatus	Init(AudioDeviceID input, AudioDeviceID output);
	void		Cleanup();
	OSStatus	Start();
	OSStatus	Stop();
	Boolean		IsRunning();
	OSStatus	SetInputDeviceAsCurrent(AudioDeviceID in);
	OSStatus	SetOutputDeviceAsCurrent(AudioDeviceID out);
	
	AudioDeviceID GetInputDeviceID()	{ return mInputDevice.mID;	}
	AudioDeviceID GetOutputDeviceID()	{ return mOutputDevice.mID; }
	
	

private:
	OSStatus SetupGraph(AudioDeviceID out);
	OSStatus MakeGraph();
	
	OSStatus SetupAUHAL(AudioDeviceID in);
	OSStatus EnableIO();
	OSStatus CallbackSetup();
	OSStatus SetupBuffers();
	
	void ComputeThruOffset();
	
	static OSStatus InputProc(void *inRefCon,
							  AudioUnitRenderActionFlags *ioActionFlags,
							  const AudioTimeStamp *inTimeStamp,
							  UInt32				inBusNumber,
							  UInt32				inNumberFrames,
							  AudioBufferList *		ioData);
	
	static OSStatus OutputProc(void *inRefCon,
							   AudioUnitRenderActionFlags *ioActionFlags,
							   const AudioTimeStamp *inTimeStamp,
							   UInt32				inBusNumber,
							   UInt32				inNumberFrames,
							   AudioBufferList *	ioData);
											
	AudioUnit mInputUnit;
	AudioBufferList *mInputBuffer;
	AudioDevice mInputDevice, mOutputDevice;
	
	//AudioUnits and Graph
	AUGraph mGraph;
	//AUNode mVarispeedNode;
	//AudioUnit mVarispeedUnit;
	AUNode mOutputNode;
	AudioUnit mOutputUnit;
	
	//Buffer sample info
	Float64 mFirstInputTime;
	Float64 mFirstOutputTime;
	Float64 mInToOutSampleOffset;
public:
	Float64 mSampleRate; //only valid after setupbuffers()
	struct device_t *xwax_device;

};


#pragma mark ---Public Methods---


#pragma mark ---CAPlayThrough Methods---
CAPlayThrough::CAPlayThrough(AudioDeviceID input, AudioDeviceID output, struct device_t *xwax_device):
mFirstInputTime(-1),
mFirstOutputTime(-1),
mInToOutSampleOffset(0),
xwax_device(xwax_device),
mSampleRate(-1)
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
	//Note: You can interface to input and output devices with "output" audio units.
	//Please keep in mind that you are only allowed to have one output audio unit per graph (AUGraph).
	//As you will see, this sample code splits up the two output units.  The "output" unit that will
	//be used for device input will not be contained in a AUGraph, while the "output" unit that will 
	//interface the default output device will be in a graph.
	
	//Setup AUHAL for an input device
	err = SetupAUHAL(input);
	checkErr(err);
	
	//Setup Graph containing Varispeed Unit & Default Output Unit
	err = SetupGraph(output);	
	checkErr(err);
	
	err = SetupBuffers();
	checkErr(err);
	
	// the varispeed unit should only be conected after the input and output formats have been set
	//err = AUGraphConnectNodeInput(mGraph, mVarispeedNode, 0, mOutputNode, 0);
	//checkErr(err);
	
	err = AUGraphInitialize(mGraph); 
	checkErr(err);
	
	//Add latency between the two devices
	ComputeThruOffset();
		
	return err;	
}

void CAPlayThrough::Cleanup()
{
	//clean up
	Stop();
									
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
		
		//reset sample times
		mFirstInputTime = -1;
		mFirstOutputTime = -1;
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
		mFirstInputTime = -1;
		mFirstOutputTime = -1;
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
    UInt32 size = sizeof(AudioDeviceID);;
    OSStatus err = noErr;
	
	if(out == kAudioDeviceUnknown) //Retrieve the default output device
	{
		err = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultOutputDevice,
									   &size,  
									   &out);
	}
	mOutputDevice.Init(out, false);
	checkErr(err);
	
	//Set the Current Device to the Default Output Unit.
    err = AudioUnitSetProperty(mOutputUnit,
							  kAudioOutputUnitProperty_CurrentDevice, 
							  kAudioUnitScope_Global, 
							  0, 
							  &mOutputDevice.mID, 
							  sizeof(mOutputDevice.mID));
							
	return err;
}

OSStatus CAPlayThrough::SetInputDeviceAsCurrent(AudioDeviceID in)
{
    UInt32 size = sizeof(AudioDeviceID);
    OSStatus err = noErr;
	
	if(in == kAudioDeviceUnknown) //get the default input device if device is unknown
	{  
		err = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultInputDevice,
									   &size,  
									   &in);
		checkErr(err);
	}
	
	mInputDevice.Init(in, true);
	
	//Set the Current Device to the AUHAL.
	//this should be done only after IO has been enabled on the AUHAL.
    err = AudioUnitSetProperty(mInputUnit,
							  kAudioOutputUnitProperty_CurrentDevice, 
							  kAudioUnitScope_Global, 
							  0, 
							  &mInputDevice.mID, 
							  sizeof(mInputDevice.mID));
	checkErr(err);
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
	//Otherwise sample rate changes will cause sync los
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
	
	err = AudioUnitSetProperty(mOutputUnit, 
							  kAudioUnitProperty_SetRenderCallback, 
							  kAudioUnitScope_Input,
							  0,
							  &output, 
							  sizeof(output));
	checkErr(err);		
	
	return err;
}

OSStatus CAPlayThrough::MakeGraph()
{
	OSStatus err = noErr;
	ComponentDescription /*varispeedDesc,*/outDesc;
	
	//Q:Why do we need a varispeed unit?
	//A:If the input device and the output device are running at different sample rates
	//we will need to move the data coming to the graph slower/faster to avoid a pitch change.
	/*
	varispeedDesc.componentType = kAudioUnitType_FormatConverter;
	varispeedDesc.componentSubType = kAudioUnitSubType_Varispeed;
	varispeedDesc.componentManufacturer = kAudioUnitManufacturer_Apple;
	varispeedDesc.componentFlags = 0;        
	varispeedDesc.componentFlagsMask = 0;     
  */
	outDesc.componentType = kAudioUnitType_Output;
	outDesc.componentSubType = kAudioUnitSubType_DefaultOutput;
	outDesc.componentManufacturer = kAudioUnitManufacturer_Apple;
	outDesc.componentFlags = 0;
	outDesc.componentFlagsMask = 0;
	
	//////////////////////////
	///MAKE NODES
	//This creates a node in the graph that is an AudioUnit, using
	//the supplied ComponentDescription to find and open that unit	
	//err = AUGraphAddNode(mGraph, &varispeedDesc, &mVarispeedNode);
	//checkErr(err);
	err = AUGraphAddNode(mGraph, &outDesc, &mOutputNode);
	checkErr(err);
	
	//Get Audio Units from AUGraph node
	//err = AUGraphNodeInfo(mGraph, mVarispeedNode, NULL, &mVarispeedUnit);   
	//checkErr(err);
	err = AUGraphNodeInfo(mGraph, mOutputNode, NULL, &mOutputUnit);   
	checkErr(err);
	
	// don't connect nodes until the varispeed unit has input and output formats set

	return err;
}

OSStatus CAPlayThrough::SetupAUHAL(AudioDeviceID in)
{
	OSStatus err = noErr;
	
	Component comp;
	ComponentDescription desc;
	
	//There are several different types of Audio Units.
	//Some audio units serve as Outputs, Mixers, or DSP
	//units. See AUComponent.h for listing
	desc.componentType = kAudioUnitType_Output;
	
	//Every Component has a subType, which will give a clearer picture
	//of what this components function will be.
	desc.componentSubType = kAudioUnitSubType_HALOutput;
	
	//all Audio Units in AUComponent.h must use 
	//"kAudioUnitManufacturer_Apple" as the Manufacturer
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
	
	///////////////
	//ENABLE IO (INPUT)
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
	printf("=====Input DEVICE stream format\n" );	
	asbd_dev1_in.Print();
	
	//Get the Stream Format (client side)
	propertySize = sizeof(asbd);
	err = AudioUnitGetProperty(mInputUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 1, &asbd, &propertySize);		
	printf("=====current Input (Client) stream format\n");	
	asbd.Print();	

	//Get the Stream Format (Output client side)
	propertySize = sizeof(asbd_dev2_out);
	err = AudioUnitGetProperty(mOutputUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &asbd_dev2_out, &propertySize);
	printf("=====Output (Device) stream format\n");	
	asbd_dev2_out.Print();
	
	//////////////////////////////////////
	//Set the format of all the AUs to the input/output devices channel count
	//For a simple case, you want to set this to the lower of count of the channels
	//in the input device vs output device
	//////////////////////////////////////
	asbd.mChannelsPerFrame =((asbd_dev1_in.mChannelsPerFrame < asbd_dev2_out.mChannelsPerFrame) ?asbd_dev1_in.mChannelsPerFrame :asbd_dev2_out.mChannelsPerFrame) ;
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
	//err = AudioUnitSetProperty(mVarispeedUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &asbd, propertySize);
	//checkErr(err);
	
	//Set the correct sample rate for the output device, but keep the channel count the same
	propertySize = sizeof(Float64);
	AudioDeviceGetProperty(mOutputDevice.mID, 0, 0, kAudioDevicePropertyNominalSampleRate, &propertySize, &outRate);
	asbd.mSampleRate =outRate;
	propertySize = sizeof(asbd);
	//Set the new audio stream formats for the rest of the AUs...
	//err = AudioUnitSetProperty(mVarispeedUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &asbd, propertySize);
	//checkErr(err);	
	err = AudioUnitSetProperty(mOutputUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &asbd, propertySize);
	checkErr(err);

	if (inRate != outRate)
	{
		fprintf(stderr, "Input rate and output rate must be the same!\n");
		return -1;
	}
	mSampleRate = inRate;
	
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
	
	
    return err;
}

void	CAPlayThrough::ComputeThruOffset()
{
	//The initial latency will at least be the saftey offset's of the devices + the buffer sizes
	mInToOutSampleOffset = SInt32(mInputDevice.mSafetyOffset +  mInputDevice.mBufferSizeFrames +
						mOutputDevice.mSafetyOffset + mOutputDevice.mBufferSizeFrames);
}

/* Stereo interleave */
#define SCALE 32768
static void interleave(signed short *buf, AudioBufferList *cabuf,
                       UInt32 nframes)
{
	AudioSampleType *l,*r;
	if (cabuf->mNumberBuffers== 2)
	{
		l = (AudioSampleType*)cabuf->mBuffers[0].mData;
		r = (AudioSampleType*)cabuf->mBuffers[1].mData;
	}
	else if (cabuf->mNumberBuffers == 1)
	{
//		printf("input mono, faking 2nd channel\n");		
		l = (AudioSampleType*)cabuf->mBuffers[0].mData;
		r = (AudioSampleType*)cabuf->mBuffers[0].mData;
	}
	else
	{
//		printf("input %d channels, ignoring extra after first two\n", cabuf->mNumberBuffers);				
		l = (AudioSampleType*)cabuf->mBuffers[0].mData;
		r = (AudioSampleType*)cabuf->mBuffers[1].mData;
	}
	
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
                       UInt32 nframes)
{
	AudioSampleType *l,*r;
	if (cabuf->mNumberBuffers== 2)
	{
		l = (AudioSampleType*)cabuf->mBuffers[0].mData;
		r = (AudioSampleType*)cabuf->mBuffers[1].mData;
	}
	else if (cabuf->mNumberBuffers == 1)
	{
//		printf("input mono, faking 2nd channel\n");		
		l = (AudioSampleType*)cabuf->mBuffers[0].mData;
		r = (AudioSampleType*)cabuf->mBuffers[0].mData;
	}
	else
	{
//		printf("input %d channels, ignoring extra after first two\n", cabuf->mNumberBuffers);				
		l = (AudioSampleType*)cabuf->mBuffers[0].mData;
		r = (AudioSampleType*)cabuf->mBuffers[1].mData;
	}
	
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
 	static UInt32 prevNumFrames = 0;
	static signed short *pcmData = NULL;
    OSStatus err = noErr;
	
	CAPlayThrough *This = (CAPlayThrough *)inRefCon;
	if (This->mFirstInputTime < 0.)
		This->mFirstInputTime = inTimeStamp->mSampleTime;
		
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
		if(This->xwax_device->timecoder)
		{
			if (prevNumFrames != inNumberFrames)
			{
				pcmData = (signed short*)realloc(pcmData, sizeof(signed short)*inNumberFrames*2); // stereo
			}
			prevNumFrames = inNumberFrames;
			interleave(pcmData, This->mInputBuffer, inNumberFrames);
            timecoder_submit(This->xwax_device->timecoder, pcmData, (size_t)inNumberFrames);
		}
	}
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
	static UInt32 prevNumFrames = 0;
	static signed short *pcmData = NULL;

	CAPlayThrough *This = (CAPlayThrough *)inRefCon;

	if (prevNumFrames != inNumberFrames)
	{
		pcmData = (signed short*)realloc(pcmData, sizeof(signed short)*inNumberFrames*2); // stereo
	}
	prevNumFrames = inNumberFrames;

	player_collect(This->xwax_device->player, pcmData, inNumberFrames, This->mSampleRate);//FIXME sample rate
	uninterleave(ioData, pcmData, inNumberFrames);
	
	return noErr;
}

#pragma mark -- CAPlayThroughHost Methods --

CAPlayThroughHost::CAPlayThroughHost(AudioDeviceID input, AudioDeviceID output, struct device_t *xwax_device):
	mPlayThrough(NULL)
{
	CreatePlayThrough(input, output, xwax_device);
}

CAPlayThroughHost::~CAPlayThroughHost()
{
	DeletePlayThrough();
}

void CAPlayThroughHost::CreatePlayThrough(AudioDeviceID input, AudioDeviceID output, struct device_t *xwax_device)
{
	mPlayThrough = new CAPlayThrough(input, output, xwax_device);
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
bool CAPlayThroughHost::PlayThroughExists()
{
	return (mPlayThrough != NULL) ? true : false;
}

OSStatus	CAPlayThroughHost::Start()
{
	if (mPlayThrough) return mPlayThrough->Start();
	return noErr;
}

OSStatus	CAPlayThroughHost::Stop()
{
	if (mPlayThrough) return mPlayThrough->Stop();
	return noErr;
}

Boolean		CAPlayThroughHost::IsRunning()
{
	if (mPlayThrough) return mPlayThrough->IsRunning();
	return noErr;
}

Float64 CAPlayThroughHost::GetSampleRate()
{
	return this->GetPlayThrough()->mSampleRate;
}