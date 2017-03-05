//***************************************************************************
// Group VDR/GraphTFT
// File common.cc
// Date 04.11.06 - Jörg Wendel
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
//***************************************************************************

#include <sys/time.h>
#include <stdarg.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

#include <plasma/applet.h>

//***************************************************************************
// Tell
//***************************************************************************

int tell(int eloquence, const char* format, ...)
{
   const int sizeTime = 8;        // "12:12:34"
   const int sizeMSec = 4;        // ",142"
   const int sizeHeader = sizeTime + sizeMSec + 1;
   const int maxBuf = 1000;

   struct timeval tp;
   char buf[maxBuf];
   va_list ap;
   time_t now;

   if (eloquence < 2)
   {
      va_start(ap, format);
      
      time(&now);
      gettimeofday(&tp, 0);
      
      vsnprintf(buf + sizeHeader, maxBuf - sizeHeader, format, ap);
      strftime(buf, sizeTime+1, "%H:%M:%S", localtime(&now));
      
      sprintf(buf+sizeTime, ",%3.3ld", tp.tv_usec / 1000);
      
      buf[sizeHeader-1] = ' ';
      
      kDebug() << buf;

      va_end(ap);
   }  

   return 0;
}
