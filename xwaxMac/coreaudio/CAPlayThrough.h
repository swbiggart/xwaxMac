/* Based on Apple sample code */

#ifndef __CAPlayThrough_H__
#define __CAPlayThrough_H__

#define checkErr( err) \
if(err) {\
    OSStatus error = static_cast<OSStatus>(err);\
        fprintf(stdout, "CAPlayThrough Error: %ld ->  %s:  %d\n",  error,\
               __FILE__, \
               __LINE__\
               );\
                   fflush(stdout);\
exit(1); \
}         

#include <CoreAudio/CoreAudio.h>
#include <AudioToolbox/AudioToolbox.h>
#include <AudioUnit/AudioUnit.h>
#include "AudioDevice.h"
#include "CAStreamBasicDescription.h"


extern "C"
{
#include "device.h"
#include "timecoder.h"
#include "player.h"
}
class CAPlayThrough;

// This class will manage the lifecycle of the play through objects
class CAPlayThroughHost
{

public:
    CAPlayThroughHost(AudioDeviceID input, AudioDeviceID output, int inChanL, int inChanR, int outChanL, int outChanR, struct device_t *xwax_device, int latency);
    ~CAPlayThroughHost();
    
    void        CreatePlayThrough(AudioDeviceID input, AudioDeviceID output, int inChanL, int inChanR, int outChanL, int outChanR, struct device_t *xwax_device, int latency);
    
    void        DeletePlayThrough();
    bool        PlayThroughExists();
    
    OSStatus    Start();
    OSStatus    Stop();
    Boolean        IsRunning();
    Float64     GetSampleRate();
    

private:
    CAPlayThrough* CAPlayThroughHost::GetPlayThrough() { return mPlayThrough; }
private:
    CAPlayThrough *mPlayThrough;
};

#endif //__CAPlayThrough_H__
