/*
 *  ReadPreferences.c
 *  xwaxMac
 *
 *  Created by Thomas Blench on 16/09/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "ReadPreferences.h"
#include <CoreFoundation/CoreFoundation.h>

// Read preferences into prefs or return -1 if not present
int readPreferences(struct prefs *prefs)
{
    //try to get some preferences if there are any
    CFStringRef notFirstTimeKey = CFSTR("notFirstTime");
    CFBooleanRef notFirstTimeValue;
    
    // Read the preference.
    notFirstTimeValue = (CFBooleanRef)CFPreferencesCopyAppValue(notFirstTimeKey, kCFPreferencesCurrentApplication);
    if (notFirstTimeValue == kCFBooleanTrue) {

        CFStringRef latencyKey = CFSTR("latency");
        CFNumberRef latencyValue = (CFNumberRef)CFPreferencesCopyAppValue(latencyKey, kCFPreferencesCurrentApplication);
        
        CFStringRef decksKey = CFSTR("decks");
        CFArrayRef array = (CFArrayRef)CFPreferencesCopyAppValue(decksKey, kCFPreferencesCurrentApplication);
        
        CFNumberGetValue(latencyValue, kCFNumberIntType, &prefs->latency);
        int decksCount = CFArrayGetCount(array);
        
        struct iopair *ios = malloc(decksCount * sizeof(struct iopair));
        
        prefs->ios = ios;
        prefs->nDecks = decksCount;

        CFStringRef timecodeKey = CFSTR("timecode");
        CFStringRef timecodeValue = (CFStringRef)CFPreferencesCopyAppValue(timecodeKey, kCFPreferencesCurrentApplication);
        CFStringGetCString(timecodeValue, prefs->timecode, 64, kCFStringEncodingASCII);
        
        CFStringRef recordDeviceNameKey = CFSTR("recordDeviceName");
        CFStringRef recordDeviceNameValue = (CFStringRef)CFPreferencesCopyAppValue(recordDeviceNameKey, kCFPreferencesCurrentApplication);
        CFStringGetCString(recordDeviceNameValue, prefs->recordDeviceName, 64, kCFStringEncodingASCII);
        
        CFStringRef recordDeviceChanLKey = CFSTR("recordDeviceChanL");
        CFNumberRef recordDeviceChanLValue = (CFNumberRef)CFPreferencesCopyAppValue(recordDeviceChanLKey, kCFPreferencesCurrentApplication);
        CFNumberGetValue(recordDeviceChanLValue, kCFNumberIntType, &prefs->recordDeviceChanL);
        
        CFStringRef recordDeviceChanRKey = CFSTR("recordDeviceChanR");
        CFNumberRef recordDeviceChanRValue = (CFNumberRef)CFPreferencesCopyAppValue(recordDeviceChanRKey, kCFPreferencesCurrentApplication);;
        CFNumberGetValue(recordDeviceChanRValue, kCFNumberIntType, &prefs->recordDeviceChanR);
        
        CFStringRef recordFormatKey = CFSTR("recordFormat");
        CFStringRef recordFormatValue = (CFStringRef)CFPreferencesCopyAppValue(recordFormatKey, kCFPreferencesCurrentApplication);
        CFStringGetCString(recordFormatValue, prefs->recordFormat, 64, kCFStringEncodingUTF8);
        
        CFStringRef recordBitRateKey = CFSTR("recordBitrate");
        CFNumberRef recordBitRateValue = (CFNumberRef)CFPreferencesCopyAppValue(recordBitRateKey, kCFPreferencesCurrentApplication);
        CFNumberGetValue(recordBitRateValue, kCFNumberIntType, &prefs->recordBitrate);
        
        CFStringRef recordPathKey = CFSTR("recordPath");
        CFStringRef recordPathValue = (CFStringRef)CFPreferencesCopyAppValue(recordFormatKey, kCFPreferencesCurrentApplication);
        CFStringGetCString(recordPathValue, prefs->recordPath, 1024, kCFStringEncodingUTF8);
        
        CFStringRef recordEnabledKey = CFSTR("recordEnabled");
        CFBooleanRef recordEnabledValue = (CFBooleanRef)CFPreferencesCopyAppValue(recordEnabledKey, kCFPreferencesCurrentApplication);
        prefs->recordEnabled = recordEnabledValue == kCFBooleanTrue ? 1 : 0;
        
        for (int i=0; i<decksCount; i++) {
            CFDictionaryRef dict = (CFDictionaryRef)CFArrayGetValueAtIndex(array, i);
            
            CFStringRef inDeviceIdKey = CFSTR("inDeviceName");
            CFStringRef inDeviceIdValue = (CFStringRef)CFDictionaryGetValue(dict, inDeviceIdKey);
            CFStringGetCString(inDeviceIdValue, ios[i].inDeviceName, 64, kCFStringEncodingASCII);
            
            CFStringRef outDeviceIdKey = CFSTR("outDeviceName");
            CFStringRef outDeviceIdValue = (CFStringRef)CFDictionaryGetValue(dict, outDeviceIdKey);
            CFStringGetCString(outDeviceIdValue, ios[i].outDeviceName, 64, kCFStringEncodingASCII);
                        
            CFStringRef inDeviceChanLKey = CFSTR("inDeviceChanL");
            CFNumberRef inDeviceChanLValue = (CFNumberRef)CFDictionaryGetValue(dict, inDeviceChanLKey);
            CFNumberGetValue(inDeviceChanLValue, kCFNumberIntType, &ios[i].inDeviceChanL);
            
            CFStringRef inDeviceChanRKey = CFSTR("inDeviceChanR");
            CFNumberRef inDeviceChanRValue = (CFNumberRef)CFDictionaryGetValue(dict, inDeviceChanRKey);
            CFNumberGetValue(inDeviceChanRValue, kCFNumberIntType, &ios[i].inDeviceChanR);
            
            CFStringRef outDeviceChanLKey = CFSTR("outDeviceChanL");
            CFNumberRef outDeviceChanLValue = (CFNumberRef)CFDictionaryGetValue(dict, outDeviceChanLKey);
            CFNumberGetValue(outDeviceChanLValue, kCFNumberIntType, &ios[i].outDeviceChanL);
            
            CFStringRef outDeviceChanRKey = CFSTR("outDeviceChanR");
            CFNumberRef outDeviceChanRValue = (CFNumberRef)CFDictionaryGetValue(dict, outDeviceChanRKey);
            CFNumberGetValue(outDeviceChanRValue, kCFNumberIntType, &ios[i].outDeviceChanR);
            
            CFRelease(inDeviceIdKey);
            CFRelease(inDeviceIdValue);
            CFRelease(outDeviceIdKey);
            CFRelease(outDeviceIdValue);
            CFRelease(inDeviceChanLKey);
            CFRelease(inDeviceChanLValue);
            CFRelease(inDeviceChanRKey);
            CFRelease(inDeviceChanRValue);
            CFRelease(outDeviceChanLKey);
            CFRelease(outDeviceChanLValue);
            CFRelease(outDeviceChanRKey);
            CFRelease(outDeviceChanRValue);                    
        }

        CFRelease(decksKey);
        CFRelease(latencyKey);
        CFRelease(latencyValue);
        CFRelease(notFirstTimeKey);
        CFRelease(notFirstTimeValue);
        CFRelease(timecodeKey);
        CFRelease(recordDeviceNameKey);
        CFRelease(recordDeviceNameValue);
        CFRelease(recordDeviceChanLKey);
        CFRelease(recordDeviceChanLValue);
        CFRelease(recordDeviceChanRKey);
        CFRelease(recordDeviceChanRValue);
        CFRelease(recordFormatKey);
        CFRelease(recordFormatValue);
        CFRelease(recordBitRateKey);
        CFRelease(recordBitRateValue);
        CFRelease(recordPathKey);
        CFRelease(recordPathValue);
        CFRelease(recordEnabledKey);
        CFRelease(recordEnabledValue);
        CFRelease(array);
            
        return 0;
    } else {
        return -1;
    }
}