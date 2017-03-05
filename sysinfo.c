/*
 * sysinfo.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * Date: 03.06.07
 *
 */

#include <stdio.h>
#include <unistd.h>

#include <glibtop.h>
#include <glibtop/cpu.h>
#include <glibtop/mem.h>

#ifndef WITHOUT_VDR
# include <vdr/tools.h>
#endif

#include <sysinfo.h>

int Sysinfo::initialized = no;
double Sysinfo::lastIdle = 0;
double Sysinfo::lastTotal = 0;

//***************************************************************************
// Init
//***************************************************************************

int Sysinfo::init()
{
   if (initialized)
      return done;

#if LIBGTOP_MAJOR_VERSION > 2 || (LIBGTOP_MAJOR_VERSION == 2 && LIBGTOP_MINOR_VERSION >= 14)
   tell(0, "init libgtop (%d.%d)", 
     LIBGTOP_MAJOR_VERSION, LIBGTOP_MINOR_VERSION);
   glibtop_init();
#endif

   initialized = yes;

   return done;
}

//***************************************************************************
// Init
//***************************************************************************

int Sysinfo::exit()
{
   if (!initialized)
      return done;

#if LIBGTOP_MAJOR_VERSION > 2 || (LIBGTOP_MAJOR_VERSION == 2 && LIBGTOP_MINOR_VERSION >= 14)
      glibtop_close();
#endif

   initialized = no;

   return done;

}

//***************************************************************************
// Get CPU Load
//***************************************************************************

int Sysinfo::cpuLoad()
{	
   static uint64_t lastCall = 0;
   static int lastLoad = na;

   if (!initialized)
      init();

   int load;
	glibtop_cpu cpu;

#ifndef WITHOUT_VDR
   if (lastCall && lastLoad != na && cTimeMs::Now() - lastCall < 200)
      return lastLoad;
#endif

	glibtop_get_cpu(&cpu);

   double total = ((unsigned long)cpu.total) ? ((double)cpu.total) : 1.0;
   double idle = ((unsigned long)cpu.idle)  ? ((double)cpu.idle)  : 1.0;

	total /= (double)cpu.frequency;
	idle  /= (double)cpu.frequency;
	
   // calc tic deltas (from last)

   double loadTics = total- idle - lastTotal;
   double idleTics = idle - lastIdle;

   // calc load

	load = (int) (loadTics * 100 / (loadTics + idleTics));

   // remember

	lastTotal = total - idle;
	lastIdle = idle;
   lastLoad = load;

#ifndef WITHOUT_VDR
   lastCall = cTimeMs::Now();
#endif

	return load;
}

//***************************************************************************
// Get Memory Info
//***************************************************************************

int Sysinfo::memInfoMb(unsigned long& total, 
                       unsigned long& used, 
                       unsigned long& free,
                       unsigned long& cached)
{
   glibtop_mem memory;

   if (!initialized)
      init();

   glibtop_get_mem(&memory);

   total = (unsigned long)memory.total/(1024*1024);
   used = (unsigned long)memory.used/(1024*1024);
   free = (unsigned long)memory.free/(1024*1024);
   cached = (unsigned long)memory.cached/(1024*1024);

   /*
   tell(4, "\nMEMORY USING\n\n"
     "Memory Total : %ld MB\n"
     "Memory Used : %ld MB\n"
     "Memory Free : %ld MB\n"
     "Memory Buffered : %ld MB\n"
     "Memory Cached : %ld MB\n"
     "Memory user : %ld MB\n"
     "Memory Locked : %ld MB\n",
     total, used, free,
     (unsigned long)memory.shared/(1024*1024),
     (unsigned long)memory.buffer/(1024*1024),
     (unsigned long)memory.cached/(1024*1024),
     (unsigned long)memory.user/(1024*1024),
     (unsigned long)memory.locked/(1024*1024));
   */
	
   return done;
} 

//***************************************************************************

