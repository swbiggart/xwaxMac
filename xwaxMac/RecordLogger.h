/*
 *  RecordLogger.h
 *  xwaxMac
 *
 *  Created by Tom Blench on 28/10/2009.
 *  Copyright 2009 Zen_Platypus. All rights reserved.
 *
 */

#include <stdio.h>
#include "library.h"

class RecordLogger
{
public:
    RecordLogger(const char *filename);
    void LogRecord(const char *t, const char *a, const char *p);
private:
    FILE *fp;
};