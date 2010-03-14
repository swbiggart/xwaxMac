/*
 *  UnicodeUtils.h
 *  xwaxMac
 *
 *  Created by Tom Blench on 09/11/2009.
 *  Copyright 2009 Zen_Platypus. All rights reserved.
 *
 */

#include <CoreFoundation/CFString.h>
#include <Carbon/Carbon.h>

static OSStatus ConvertUnicodeToCanonical(
                                          Boolean precomposed,
                                          const UniChar *inputBuf, ByteCount inputBufLen,
                                          UniChar *outputBuf, ByteCount outputBufSize,
                                          ByteCount *outputBufLen);