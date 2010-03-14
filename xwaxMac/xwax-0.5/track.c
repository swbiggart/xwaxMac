/*
 * Copyright (C) 2009 Mark Hills <mark@pogo.org.uk>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "track.h"
#include "AudioFile.h"

#define SAMPLE 4 /* bytes per sample (all channels) */

#define LOCK(tr)  pthread_mutex_lock(&(tr)->mx)
#define UNLOCK(tr)  pthread_mutex_unlock(&(tr)->mx)


/* Start the importer process. On completion, pid and fd are set */

static int start_import(struct track_t *tr, const char *path)
{
    int pstdout[2];

    if(pipe(pstdout) == -1) {
        perror("pipe");
        return -1;
    }
    
    tr->pid = vfork();
    
    if(tr->pid < 0) {
        perror("vfork");
        return -1;
        
    } else if(tr->pid == 0) { /* child */

        close(pstdout[0]);
        dup2(pstdout[1], STDOUT_FILENO);
        close(pstdout[1]);
        
        if(execl(tr->importer, "import", path, NULL) == -1) {
            perror("execl");
            exit(-1);
            return 0;
        }
    }

    close(pstdout[1]);
    tr->fd = pstdout[0];
    if(fcntl(tr->fd, F_SETFL, O_NONBLOCK) == -1) {
        perror("fcntl");
        return -1;
    }

    tr->bytes = 0;
    tr->length = 0;
    tr->ppm = 0;
    tr->overview = 0;
    tr->eof = 0;
    tr->rate = TRACK_RATE;

    return 0;
}


/* Conclude the importer process. To be called whether the importer
 * was aborted or completed successfully */

static int stop_import(struct track_t *tr)
{
    int status;

    if(close(tr->fd) == -1) {
        perror("close");
        return -1;
    }

    if(waitpid(tr->pid, &status, 0) == -1) {
        perror("waitpid");
        return -1;
    }
    fprintf(stderr, "Track importer exited with status %d.\n", status);

    return 0;
}

/* Read from buffer, osx.  Buffer size in bytes */

int read_from_buffer(struct track_t *tr)
{
    int ls;
    size_t m;
    unsigned short v;
    unsigned int s, w;
    struct track_block_t *block;
		
	
	
	unsigned int b;
	size_t offset=0; //offset into buffer
	

	
	
	
	for(b=0;;b++)
	{       
		if (tr->blocks >= tr->oldblocks)
		{
			printf("Block %u, offset %ld\n", b, offset);
			if(tr->blocks >= TRACK_MAX_BLOCKS) {
				fprintf(stderr, "Maximum track length reached; aborting...\n");
				return -1;
			}
			block = malloc(sizeof(struct track_block_t));
			if(!block) {
				perror("malloc");
				return -1;
			}		
			tr->block[tr->blocks++] = block;
		}
		else
		{
			block = tr->block[tr->blocks++];
		}
		// Fill block
		size_t nbytes = TRACK_BLOCK_SAMPLES * SAMPLE;
		size_t left = tr->bufsiz - offset;
		if (nbytes > left)
		{
			nbytes = left;
		}		
		printf("left %ld, nbytes %ld\n", left, nbytes);
		memcpy((char*)block->pcm, ((char*)tr->buf)+offset, nbytes);
		offset+=nbytes;
		if (offset == tr->bufsiz)
		{
			break;
		}		
	}
	
	printf("Blocks=%d\n",b);
	
	// If there are less blocks than last time, free them
	for (int i=tr->blocks;i<tr->oldblocks;i++)
	{
		free(tr->block[i]);
	}
	
	tr->bytes = tr->bufsiz;
	
        m = TRACK_BLOCK_SAMPLES * SAMPLE * tr->blocks / 1024;
        fprintf(stderr, "Track memory %zuKb PCM, %zuKb PPM, %zuKb overview.\n",
                m, m / TRACK_PPM_RES, m / TRACK_OVERVIEW_RES);
		
    
	
    /* Meter the audio which has just been read */
    
    for(s = 0; s < tr->bytes / SAMPLE; s++) {
        
		block = tr->block[s / (TRACK_BLOCK_SAMPLES)];
		
		ls = s % TRACK_BLOCK_SAMPLES;
        
        v = (abs(block->pcm[ls * TRACK_CHANNELS])
             + abs(block->pcm[ls * TRACK_CHANNELS]));
		
        /* PPM-style fast meter approximation */
		
        if(v > tr->ppm)
            tr->ppm += (v - tr->ppm) >> 3;
        else
            tr->ppm -= (tr->ppm - v) >> 9;
        
        block->ppm[ls / TRACK_PPM_RES] = tr->ppm >> 8;
        
        /* Update the slow-metering overview. Fixed point arithmetic
         * going on here */
		
        w = v << 16;
        
        if(w > tr->overview)
            tr->overview += (w - tr->overview) >> 8;
        else
            tr->overview -= (tr->overview - w) >> 17;
		
        block->overview[ls / TRACK_OVERVIEW_RES] = tr->overview >> 24;
    }
	printf("Block=%d\n",s / (TRACK_BLOCK_SAMPLES * SAMPLE));    
    tr->length = s;	
    return 0;
}

/* Read the next block of data from the file. Return -1 when an error
 * occurs and requires our attention, 1 if there is no more data to be
 * read, otherwise zero. */

static int read_from_pipe(struct track_t *tr)
{
    int r, ls, used;
    size_t m;
    unsigned short v;
    unsigned int s, w;
    struct track_block_t *block;

    /* Check whether we need to allocate a new block */
    
    if(tr->bytes >= (size_t)tr->blocks * TRACK_BLOCK_SAMPLES * SAMPLE) {

        if(tr->blocks >= TRACK_MAX_BLOCKS) {
            fprintf(stderr, "Maximum track length reached; aborting...\n");
            return -1;
        }
                
        block = malloc(sizeof(struct track_block_t));
        if(!block) {
            perror("malloc");
            return -1;
        }

        tr->block[tr->blocks++] = block;

        fprintf(stderr, "Allocated new track block (%d at %zu bytes).\n",
                tr->blocks, tr->bytes);
    }

    /* Load in audio to the end of the current block. We've just
     * allocated a new one if needed, so no possibility of read()
     * returning zero, except for EOF */

    block = tr->block[tr->bytes / (TRACK_BLOCK_SAMPLES * SAMPLE)];
    used = tr->bytes % (TRACK_BLOCK_SAMPLES * SAMPLE);
    
    r = read(tr->fd, (char*)block->pcm + used,
             TRACK_BLOCK_SAMPLES * SAMPLE - used);

    if(r == -1) {
        if(errno == EAGAIN)
            return 0;
        perror("read");
        return -1;

    } else if(r == 0) {
        m = TRACK_BLOCK_SAMPLES * SAMPLE * tr->blocks / 1024;
        fprintf(stderr, "Track memory %zuKb PCM, %zuKb PPM, %zuKb overview.\n",
                m, m / TRACK_PPM_RES, m / TRACK_OVERVIEW_RES);

        return 1;
    }
    
    tr->bytes += r;

    /* Meter the audio which has just been read */
    
    for(s = tr->length; s < tr->bytes / SAMPLE; s++) {
        ls = s % TRACK_BLOCK_SAMPLES;
        
        v = (abs(block->pcm[ls * TRACK_CHANNELS])
             + abs(block->pcm[ls * TRACK_CHANNELS]));

        /* PPM-style fast meter approximation */

        if(v > tr->ppm)
            tr->ppm += (v - tr->ppm) >> 3;
        else
            tr->ppm -= (tr->ppm - v) >> 9;
        
        block->ppm[ls / TRACK_PPM_RES] = tr->ppm >> 8;
        
        /* Update the slow-metering overview. Fixed point arithmetic
         * going on here */

        w = v << 16;
        
        if(w > tr->overview)
            tr->overview += (w - tr->overview) >> 8;
        else
            tr->overview -= (tr->overview - w) >> 17;

        block->overview[ls / TRACK_OVERVIEW_RES] = tr->overview >> 24;
    }
    
    tr->length = s;

    return 0;
}


void track_init(struct track_t *tr, const char *importer)
{
    tr->importer = importer;
    tr->pid = 0;

    tr->artist = NULL;
    tr->title = NULL;
    tr->name = NULL;

    tr->blocks = 0;
    tr->bytes = 0;
    tr->length = 0;
    tr->rate = TRACK_RATE;

    pthread_mutex_init(&tr->mx, 0); /* always returns zero */
    pthread_mutex_init(&tr->import_mx, 0); /* always returns zero */

    tr->status = TRACK_STATUS_VALID;
}


/* Destroy this track from memory, and any child process */

int track_clear(struct track_t *tr)
{
    int n;

    /* Force a cleanup of whichever state we are in */

    if(tr->status > TRACK_STATUS_VALID) {
        if(tr->status == TRACK_STATUS_IMPORTING) {
            if(kill(tr->pid, SIGKILL) == -1) {
                perror("kill");
                return -1;
            }
        }
        if(stop_import(tr) == -1)
            return -1;
    }

    for(n = 0; n < tr->blocks; n++)
        free(tr->block[n]);

    if(pthread_mutex_destroy(&tr->mx) == EBUSY) {
        fprintf(stderr, "Track busy on destroy.\n");
        return -1;
    }

    return 0;
}


/* Return the number of file descriptors which should be watched for
 * this track, and fill pe */

int track_pollfd(struct track_t *tr, struct pollfd *pe)
{
    int r;

    LOCK(tr);

    if(tr->status == TRACK_STATUS_IMPORTING && !tr->eof) {
        pe->fd = tr->fd;
        pe->revents = 0;
        pe->events = POLLIN | POLLHUP | POLLERR;
        tr->pe = pe;
        r = 1;
    } else {
        tr->pe = NULL;
        r = 0;
    }

    UNLOCK(tr);
    return r;
}


/* Handle any activity on this track, whatever the current state */

int track_handle(struct track_t *tr)
{
    int r;

    /* Only one thread is allowed to call this function, and it owns
     * the poll entry */

    if(!tr->pe || !tr->pe->revents)
        return 0;

    LOCK(tr);

    if(tr->status == TRACK_STATUS_IMPORTING && !tr->eof) {
        r = read_from_pipe(tr);
        if(r != 0)
            tr->eof = 1;
    }

    UNLOCK(tr);
    return 0;
}

void *track_import_osx_thread(void *args)
{
	struct track_t *tr = (struct track_t*)args;
	tr->status = TRACK_STATUS_IMPORTING;
	tr->bytes = 0;
    tr->length = 0;
    tr->ppm = 0;
    tr->overview = 0;
    tr->eof = 0;
    tr->rate = TRACK_RATE;	
	tr->oldblocks = tr->blocks;
	tr->blocks = 0;
	int result = loadAudioFile(tr->path, tr);	
    if (result != 0)
    {
        printf("Import failed\n");
    }
    else
    {
    	tr->status = TRACK_STATUS_VALID;
    }
    // Unlock track, whether it succeeded or not
    pthread_mutex_unlock(&(tr)->import_mx);
    printf("Unlock import mutex for track\n");
	UNLOCK(tr);
	rig_awaken(tr->rig);
	return 0;
}
int track_import_osx(struct track_t *tr, const char *path)
{
	pthread_attr_t tattr;
	
	int ret;
	
	struct sched_param param;
	
	
	printf("About to get lock\n");
	if(pthread_mutex_trylock(&(tr)->import_mx)!=0)
	{
		printf("Not importing, as import already in progress\n");
		return -1;
	}
	
	tr->path = path;
	
	/* initialized with default attributes */
	ret = pthread_attr_init (&tattr);
	
	/* safe to get existing scheduling param */
	ret = pthread_attr_getschedparam (&tattr, &param);
	
	/* set the priority; others are unchanged */
	param.sched_priority = 0;
	
	/* setting the new scheduling param */
	ret = pthread_attr_setschedparam (&tattr, &param);
	

	pthread_create(&tr->thread, &tattr, &track_import_osx_thread, tr);
	pthread_detach(tr->thread);
    return 0;

}

/* A request to begin importing a new track. Can be called when the
 * track is in any state */

int track_import(struct track_t *tr, const char *path)
{
    int r;

    LOCK(tr);

    /* Abort any running import process */

    if(tr->status != TRACK_STATUS_VALID) {
        if(tr->status == TRACK_STATUS_IMPORTING) {
            if(kill(tr->pid, SIGTERM) == -1) {
                perror("kill");
                UNLOCK(tr);
                return -1;
            }
        }
        if(stop_import(tr) == -1) {
            UNLOCK(tr);
            return -1;
        }
    }

    /* Start the new import process */

    r = start_import(tr, path);
    if(r < 0) {
        UNLOCK(tr);
        return -1;
    }
    tr->status = TRACK_STATUS_IMPORTING;

    UNLOCK(tr);

    rig_awaken(tr->rig);

    return 0;
}


/* Do any waiting for completion of importer process if needed */

int track_wait(struct track_t *tr)
{
    LOCK(tr);

    if(tr->status == TRACK_STATUS_IMPORTING && tr->eof) {
        stop_import(tr);
        tr->status = TRACK_STATUS_VALID;
    }

    UNLOCK(tr);
    return 0;
}
