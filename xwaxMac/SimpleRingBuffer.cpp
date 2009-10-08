/*
 *  SimpleRingBuffer.cpp
 *  xwaxMac
 *
 *  Created by Tom Blench on 05/09/2009.
 *  Copyright 2009 Zen_Platypus. All rights reserved.
 *
 */

#include "SimpleRingBuffer.h"

SimpleRingBuffer::SimpleRingBuffer()
{
    m_bufsiz = 8192;
    m_buf = (AudioSampleType*)malloc(m_bufsiz * sizeof(AudioSampleType));
    state = -1;
}

SimpleRingBuffer::~SimpleRingBuffer()
{
    free(m_buf);
}

void SimpleRingBuffer::reinit()
{
    state = -1;
}

int SimpleRingBuffer::fetch(UInt32 samples, AudioSampleType *buf)
{
    int i;
    int distance = (m_store_pos % m_bufsiz - m_fetch_pos % m_bufsiz);
    
    if (distance < 0){distance += m_bufsiz;}

    if (state == -1) {
        printf("Not ready yet!\n");
        return -1;
    }
    
    if (samples > distance) {
        printf("Trying to fetch more than we can %ld > %d\n", samples,distance);
        return -1;
    }

    for (i=0;i<samples;i++) {
        buf[i] = m_buf[m_fetch_pos++ % m_bufsiz];
    }
    m_fetch_pos%=m_bufsiz;

    return 0;
}

int SimpleRingBuffer::store(UInt32 samples, AudioSampleType *buf)
{
    int i;
    
    int distance = (m_store_pos %m_bufsiz - m_fetch_pos % m_bufsiz);
    
    if (state == -1) {
        m_fetch_pos = 0;
        m_store_pos = 0;
        state = 0;
    }
    
    if (distance < 0){distance += m_bufsiz;}
    
    if (samples > m_bufsiz - distance) {
        printf("Trying to store more than we can %ld > %d\n", samples, distance);
        return -1;
    }

    for (i=0;i<samples;i++) {
        m_buf[m_store_pos++ % m_bufsiz] = buf[i];
    }
    m_store_pos%=m_bufsiz;

    return 0;    
}