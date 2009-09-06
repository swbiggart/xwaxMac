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
	pthread_mutex_init(&mx, NULL);
	state = -1;
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
//	printf("Fetching, distance = %d\n", distance);

	if (state == -1)
	{
		printf("Not ready yet!\n");
		return -1;
	}
	
	if (samples > distance)
	{
		printf("Trying to fetch more than we can %ld > %d\n", samples,distance);
		return -1;
	}


//	printf("Fetching from %ld ", m_fetch_pos % m_bufsiz);
	for (i=0;i<samples;i++)
	{
		buf[i] = m_buf[m_fetch_pos++ % m_bufsiz];
	}
	m_fetch_pos%=m_bufsiz;
//	printf("to %ld\n ", m_fetch_pos);
//	pthread_mutex_lock(&mx);
//	m_stored -= samples;
//	pthread_mutex_unlock(&mx);
	return 0;
}
int SimpleRingBuffer::store(UInt32 samples, AudioSampleType *buf)
{
	int i;
	
	if (state == -1)
	{
		//		m_stored = 0;
		m_fetch_pos = 0;
		m_store_pos = 0;
		state = 0;
	}
	
	int distance = (m_store_pos %m_bufsiz - m_fetch_pos % m_bufsiz);
	if (distance < 0){distance += m_bufsiz;}
//	printf("Storing, distance = %d\n", distance);
	

	
	if (samples > m_bufsiz - distance)
	{
		printf("Trying to store more than we can %ld > %d\n", samples, distance);
		return -1;
	}


//	printf("Storing from %ld ", m_store_pos % m_bufsiz);
	for (i=0;i<samples;i++)
	{
		m_buf[m_store_pos++ % m_bufsiz] = buf[i];
	}
//	printf("to %ld\n ", m_store_pos);
	m_store_pos%=m_bufsiz;
//	pthread_mutex_lock(&mx);
//	m_stored += samples;
//	pthread_mutex_unlock(&mx);
	return 0;
	
}