/*
 *  SimpleRingBuffer.h
 *  xwaxMac
 *
 *  Created by Tom Blench on 05/09/2009.
 *  Copyright 2009 Zen_Platypus. All rights reserved.
 *
 */

#include <CoreAudio/CoreAudio.h>
#include <pthread.h>

class SimpleRingBuffer
{
public:
    int fetch(UInt32 samples, AudioSampleType *buf);
    int store(UInt32 samples, AudioSampleType *buf);
    SimpleRingBuffer();
    ~SimpleRingBuffer();
    void reinit();
private:
    AudioSampleType *m_buf;
    UInt32 m_bufsiz;
    UInt32 m_fetch_pos;
    UInt32 m_store_pos;
    pthread_mutex_t mx;
    int state; // -1 not init, 0 init
};