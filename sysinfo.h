/*
 * sysinfo.h: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * Date: 03.06.07
 *
 */

#ifndef __SYSINFO_H
#define __SYSINFO_H

#include <common.h>

//***************************************************************************
// Sysinfo
//***************************************************************************

class Sysinfo
{
   public:

      // interface

      static int init();
      static int exit();
      static int cpuLoad();
      static int memInfoMb(unsigned long& total, unsigned long& used, 
                           unsigned long& free, unsigned long& cached);

   private:

      static double lastIdle;
      static double lastTotal;
      static int initialized;
};

//***************************************************************************
#endif //  __SYSINFO_H
