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

//#define WITH_JACK

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "alsa.h"
#include "coreaudio.h"
#include "device.h"
#include "interface.h"
#include "jack.h"
#include "library.h"
#include "listing.h"
#include "player.h"
#include "rig.h"
#include "timecoder.h"
#include "track.h"
#include "xwax.h"

#include "ReadPreferences.h"
#include "InterfaceC.h"

#include "SDL.h"

#define MAX_DECKS 3

#define DEFAULT_OSS_BUFFERS 8
#define DEFAULT_OSS_FRAGMENT 7

#define DEFAULT_ALSA_BUFFER 8 /* milliseconds */

#define DEFAULT_RATE 44100

#define DEFAULT_IMPORTER "xwax_import"
#define DEFAULT_TIMECODE "serato_2a"


/* We don't use the full flexibility of a rig, and just have a direct
 * correspondence between a device, track, player and timecoder */

struct deck_t {
    struct device_t device;
    struct track_t track;
    struct player_t player;
    struct timecoder_t timecoder;
};


static int deck_init(struct deck_t *deck, const char *timecode,
                     const char *importer, unsigned int sample_rate)
{
    if(timecoder_init(&deck->timecoder, timecode, sample_rate) == -1)
        return -1;
    track_init(&deck->track, importer);
    player_init(&deck->player);
    player_connect_track(&deck->player, &deck->track);    
    return 0;
}


static void deck_clear(struct deck_t *deck)
{
    track_clear(&deck->track);
    timecoder_clear(&deck->timecoder);
    player_clear(&deck->player);
    device_clear(&deck->device);
}


static void connect_deck_to_interface(struct interface_t *iface, int n,
                                      struct deck_t *deck)
{
    iface->timecoder[n] = &deck->timecoder;
    iface->player[n] = &deck->player;
}


static void connect_deck_to_rig(struct rig_t *rig, int n, struct deck_t *deck)
{
    rig->device[n] = &deck->device;
    rig->track[n] = &deck->track;
    rig->player[n] = &deck->player;
    rig->timecoder[n] = &deck->timecoder;
}


void usage(FILE *fd)
{
    fprintf(fd, "Usage: xwax [<parameters>]\n\n"
      "  -l <directory> Directory to scan for audio tracks\n"
      "  -t <name>      Timecode name\n"
      "  -i <program>   Specify external importer (default '%s')\n"
      "  -h             Display this message\n\n",
      DEFAULT_IMPORTER);

    fprintf(fd, "OSS device options:\n"
      "  -d <device>    Build a deck connected to OSS audio device\n"
      "  -r <hz>        Sample rate (default %dHz)\n"
      "  -b <n>         Number of buffers (default %d)\n"
      "  -f <n>         Buffer size to request (2^n bytes, default %d)\n\n",
      DEFAULT_RATE, DEFAULT_OSS_BUFFERS, DEFAULT_OSS_FRAGMENT);

#ifdef WITH_ALSA
    fprintf(fd, "ALSA device options:\n"
      "  -a <device>    Build a deck connected to ALSA audio device\n"
      "  -r <hz>        Sample rate (default %dHz)\n"
      "  -m <ms>        Buffer time (default %dms)\n\n",
      DEFAULT_RATE, DEFAULT_ALSA_BUFFER);
#endif

#ifdef WITH_JACK
    fprintf(fd, "JACK device options:\n"
      "  -j <name>      Create a JACK deck with the given name\n\n");
#endif

    fprintf(fd, "Device options, -t and -i apply to subsequent devices.\n"
      "Decks and audio directories can be specified multiple times.\n\n"
      "Available timecodes (for use with -t):\n"
      "  serato_2a (default), serato_2b, serato_cd,\n"
      "  traktor_a, traktor_b, mixvibes_v2\n\n"
      "eg. Standard 2-deck setup\n"
      "  xwax -l ~/music -d /dev/dsp -d /dev/dsp1\n\n"
      "eg. Use a larger buffer on a third deck\n"
      "  xwax -l ~/music -d /dev/dsp -d /dev/dsp1 -f 10 -d /dev/dsp2\n\n");

#ifdef WITH_ALSA
    fprintf(fd, "eg. Use OSS and ALSA devices simultaneously\n"
            "  xwax -l ~/music -d /dev/dsp -a hw:1\n\n");
#endif

#ifdef WITH_JACK
    fprintf(fd, "eg. Use OSS and JACK devices simultaneously\n"
            "  xwax -l ~/music -d /dev/dsp -j deck0\n\n");
#endif
}


int main(int argc, char *argv[])
{
    int r, n, decks, oss_fragment, oss_buffers, rate, alsa_buffer;
    char *endptr, *timecode, *importer;

    struct deck_t deck[MAX_DECKS];
    struct rig_t rig;
    struct interface_t iface;
    struct library_t library;
    struct listing_t listing;
    struct device_t *device;
    
    fprintf(stderr, BANNER "\n\n" NOTICE "\n\n");
    
    interface_init(&iface);
    rig_init(&rig);
    library_init(&library);
    
    decks = 0;
    oss_fragment = DEFAULT_OSS_FRAGMENT;
    oss_buffers = DEFAULT_OSS_BUFFERS;
    rate = DEFAULT_RATE;
    alsa_buffer = DEFAULT_ALSA_BUFFER;
    importer = DEFAULT_IMPORTER;
    timecode = DEFAULT_TIMECODE;

    struct prefs prefs;
    // Read preferences, if they exist
    int result = readPreferences(&prefs);
    if (result == 0)
    {
        // Check that the device still exists after reading prefs
        for (int i=0; i<prefs.nDecks; i++){
            int inDevId = coreaudio_id_for_device(prefs.ios[i].inDeviceName, 1);
            int outDevId = coreaudio_id_for_device(prefs.ios[i].outDeviceName, 0);
            if (inDevId == -1 || outDevId == -1) {
                // Error
                fprintf(stderr, "Warning, device %s or %s doesn't exist anymore\n", prefs.ios[i].inDeviceName, prefs.ios[i].outDeviceName);
                result = -1;
            } else {
                prefs.ios[i].inDeviceId = inDevId;
                prefs.ios[i].outDeviceId = outDevId;
            }
        }
        // Get record device
        prefs.recordDeviceId = coreaudio_id_for_device(prefs.recordDeviceName, 1);
    }
    if (result == -1) {
        // Otherwise show the preferences window
        int result = showPrefsWindow(&prefs);
        if (result == -1) {
            fprintf(stderr, "Error, couldn't read preferences or get your settings from the dialog\n");
            return -1;
        }
        else if (result == -2) { // cancel
            fprintf(stderr, "User cancelled\n");
            return -2;
        }
    }
    
    timecode = prefs.timecode;

    for (int i=0; i<prefs.nDecks; i++)    
    {
        device = &deck[decks].device;

    r = coreaudio_init(device, prefs.ios[i].inDeviceId, prefs.ios[i].inDeviceChanL, prefs.ios[i].inDeviceChanR, 
                               prefs.ios[i].outDeviceId, prefs.ios[i].outDeviceChanL, prefs.ios[i].outDeviceChanR, prefs.latency);
    if(r == -1)
    return -1;

    unsigned int sample_rate = device_sample_rate(device);

    if(deck_init(&deck[decks], timecode, importer, sample_rate) == -1)
    return -1;

    /* The timecoder and player are driven by requests from
     * the audio device */

    device_connect_timecoder(device, &deck[decks].timecoder);
    device_connect_player(device, &deck[decks].player);

    /* The rig and interface keep track of everything whilst
     * the program is running */

    connect_deck_to_interface(&iface, decks, &deck[decks]);
    connect_deck_to_rig(&rig, decks, &deck[decks]);

    decks++;
    }
    if(decks == 0) {
        fprintf(stderr, "You need to give at least one audio device to use "
                "as a deck; try -h.\n");
        return -1;
    }

    showLoadingWindow();
    // thru itunes, 2nd arg not used
    library_import(&library, "");
    hideLoadingWindow();
    iface.players = decks;
    iface.timecoders = decks;

    /* Connect everything up. Do this after selecting a timecode and
     * built the lookup tables. */

    for(n = 0; n < decks; n++)
        player_connect_timecoder(&deck[n].player, &deck[n].timecoder);
    
    // Start recording
    coreaudio_setup_record(&prefs);
    
    fprintf(stderr, "Indexing music library...\n");
    listing_init(&listing);
    if(listing_add_library(&listing, &library) == -1)
        return -1;
    listing_sort(&listing);
    iface.listing = &listing;
    
    fprintf(stderr, "Starting threads...\n");
    if(rig_start(&rig) == -1)
        return -1;

    fprintf(stderr, "Entering interface...\n");
    interface_run(&iface);
    
    fprintf(stderr, "Exiting cleanly...\n");

    if(rig_stop(&rig) == -1)
        return -1;
    
    for(n = 0; n < decks; n++)
        deck_clear(&deck[n]);
    
    timecoder_free_lookup();
    listing_clear(&listing);
    library_clear(&library);
    
    fprintf(stderr, "Done.\n");
    
    return 0;
}
