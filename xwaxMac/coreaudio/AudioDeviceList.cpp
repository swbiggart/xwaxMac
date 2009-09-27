/* Based on Apple sample code */

#include "AudioDeviceList.h"
#include "AudioDevice.h"

AudioDeviceList::AudioDeviceList(bool inputs) :
    mInputs(inputs)
{
    BuildList();
}

AudioDeviceList::~AudioDeviceList()
{
}

struct UniqueName
{
    char name[64];
    int count;
};

void    AudioDeviceList::BuildList()
{
    mDevices.clear();
    
    UInt32 propsize;
    
    verify_noerr(AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices, &propsize, NULL));
    int nDevices = propsize / sizeof(AudioDeviceID);    
    AudioDeviceID *devids = new AudioDeviceID[nDevices];
    verify_noerr(AudioHardwareGetProperty(kAudioHardwarePropertyDevices, &propsize, devids));
    
    std::vector<struct UniqueName> uniquenames;
    
    for (int i = 0; i < nDevices; ++i) {
        AudioDevice dev(devids[i], mInputs);
        if (dev.CountChannels() > 0) {
            Device d;
            d.mID = devids[i];
            d.mChannels = dev.CountChannels();
            dev.GetName(d.mName, sizeof(d.mName));
            bool found = false;
            int nth;
            for (int j=0; j<uniquenames.size();j++) {
                if (!strcmp(uniquenames[j].name, d.mName)) {
//                    printf("Found %s in unique\n", d.mName);
                    nth = ++(uniquenames[j].count);
                    found = true;
                }
            }
            if (found == false) {
                struct UniqueName newunique;
//                printf("Added %s to unique\n", d.mName);
                memcpy(newunique.name,d.mName,64);
                newunique.count = 1;
                uniquenames.push_back(newunique);
            }
            else {
                sprintf(d.mName,"%s %d", d.mName, nth);
            }
            mDevices.push_back(d);
        }
    }
    delete[] devids;
    
}


