/*
 *  UnicodeUtils.c
 *  xwaxMac
 *
 *  Created by Tom Blench on 09/11/2009.
 *  Copyright 2009 Zen_Platypus. All rights reserved.
 *
 */

#include "UnicodeUtils.h"

static OSStatus ConvertUnicodeToCanonical(
                                          Boolean precomposed,
                                          const UniChar *inputBuf, ByteCount inputBufLen,
                                          UniChar *outputBuf, ByteCount outputBufSize,
                                          ByteCount *outputBufLen)
/* As is standard with the Unicode Converter,
 all lengths are in bytes. */
{
    OSStatus            err;
    OSStatus            junk;
    TextEncodingVariant variant;
    UnicodeToTextInfo   uni;
    UnicodeMapping      map;
    ByteCount           junkRead;
    
    assert(inputBuf     != NULL);
    assert(outputBuf    != NULL);
    assert(outputBufLen != NULL);
    
    if (precomposed) {
        variant = kUnicodeCanonicalCompVariant;
    } else {
        variant = kUnicodeCanonicalDecompVariant;
    }
    map.unicodeEncoding = CreateTextEncoding(kTextEncodingUnicodeDefault,
                                             kUnicodeNoSubset,
                                             kTextEncodingDefaultFormat);
    map.otherEncoding   = CreateTextEncoding(kTextEncodingUnicodeDefault,
                                             variant,
                                             kTextEncodingDefaultFormat);
    map.mappingVersion  = kUnicodeUseLatestMapping;
    
    uni = NULL;
    
    err = CreateUnicodeToTextInfo(&map, &uni);
    if (err == noErr) {
        err = ConvertFromUnicodeToText(uni, inputBufLen, inputBuf,
                                       kUnicodeDefaultDirectionMask,
                                       0, NULL, NULL, NULL,
                                       outputBufSize, &junkRead,
                                       outputBufLen, outputBuf);
    }
    
    if (uni != NULL) {
        junk = DisposeUnicodeToTextInfo(&uni);
        assert(junk == noErr);
    }
    
    return err;
}