/*
 *  RecordLogger.cpp
 *  xwaxMac
 *
 *  Created by Tom Blench on 28/10/2009.
 *  Copyright 2009 Zen_Platypus. All rights reserved.
 *
 */

#include "RecordLogger.h"
#include <time.h>

RecordLogger::RecordLogger(const char *filename)
{
    this->fp = fopen(filename, "a");
}

void RecordLogger::LogRecord(const char *t, const char *a, const char *p)
{
    time_t rawtime;
    struct tm * timeinfo;
    char buf [128];
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    strftime (buf,128,"%Y-%m-%dT%H:%M:%S",timeinfo);
    
    fprintf(fp,"%s\t%s\t%s\t%s\n",buf,t,a,p);
    fflush(fp);
}