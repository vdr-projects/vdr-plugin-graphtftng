/**
 *  GraphTFTng plugin for the Video Disk Recorder 
 * 
 *  common.h - A plugin for the Video Disk Recorder
 *
 *  (c) 2004 Lars Tegeler, Sascha Volkenandt
 *  (c) 2006-2014 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 **/

#ifndef ___COMMON_H
#define ___COMMON_H

//***************************************************************************
// Includes
//***************************************************************************

#include <string>
#include <algorithm> 

#include <string.h>
#include <iconv.h>
#include <stdint.h>
#include <time.h>

class MemoryStruct;

//***************************************************************************
// 
//***************************************************************************


using std::string;

//***************************************************************************
// 
//***************************************************************************

enum TimeConst
{
   tmeSecondsPerMinute = 60,
   tmeSecondsPerHour = tmeSecondsPerMinute * 60,
   tmeSecondsPerDay = tmeSecondsPerHour *24
};

enum Misc
{
   success = 0,
   done    = success,
   fail    = -1,
   ignore  = -1,
   na      = -1,
   yes     = 1,
   on      = 1,
   off     = 0,
   no      = 0,
   TB      = 1
};

enum Sizes
{
   sizeStamp = 14,
   sizeTime = 6,
   sizeDate = 8,
   sizeHHMM = 4
};

enum LogDevice
{
   devNone,       // 0
   devStdOut,     // 1
   devSyslog,     // 2
   devFile        // 3
};

enum Eloquence
{
   eloOff,               // 0
   eloAlways,            // 1
   eloDetail,            // 2
   eloDebug,             // 3
   eloDebug1 = eloDebug, // 3
   eloDebug2,            // 4
   eloDebug3             // 5
};

extern int logLevel;
extern int logDevice;

void __attribute__ ((format(printf, 2, 3))) tell(int eloquence, const char* format, ...);

char* srealloc(void* ptr, size_t size);
const char* suffixOf(const char* path);
int fileExists(string filename);
int loadFromFile(const char* infile, MemoryStruct* data);
int jpegDimensions(const char* path, unsigned int& pWidth, unsigned int& pHeight);

#ifdef VDR_PLUGIN
int toUTF8(char* out, int outMax, const char* in);
#endif

uint64_t msNow();

//***************************************************************************
// MemoryStruct
//***************************************************************************

struct MemoryStruct
{
   public:

      MemoryStruct()   { expireAt = 0; memory = 0; zmemory = 0; clear(); }
      MemoryStruct(const MemoryStruct* o)
      {
         size = o->size;
         memory = (char*)malloc(size);
         memcpy(memory, o->memory, size);

         zsize = o->zsize;
         zmemory = (char*)malloc(zsize);
         memcpy(zmemory, o->zmemory, zsize);

         copyAttributes(o);
      }
      
      ~MemoryStruct()  { clear(); }

      int isEmpty()  { return memory == 0; }

      int append(const char* buf, int len)
      {
         memory = srealloc(memory, size+len);
         memcpy(memory+size, buf, len);
         size += len;

         return success;
      }

      void copyAttributes(const MemoryStruct* o)
      {
         strcpy(tag, o->tag);
         strcpy(name, o->name);
         strcpy(contentType, o->contentType);
         strcpy(contentEncoding, o->contentEncoding);
         strcpy(mimeType, o->mimeType);
         headerOnly = o->headerOnly;
         modTime = o->modTime;
         expireAt = o->expireAt;
      }

      void clear() 
      {
         free(memory);
         memory = 0;
         size = 0;
         free(zmemory);
         zmemory = 0;
         zsize = 0;
         *tag = 0;
         *name = 0;
         *contentType = 0;
         *contentEncoding = 0;
         *mimeType = 0;
         modTime = time(0);
         headerOnly = no;
         // expireAt = time(0); -> don't reset 'expireAt' here !!!!
      }

      // data
      
      char* memory;
      long unsigned int size;

      char* zmemory;
      long unsigned int zsize;
      
      // tag attribute
      
      char tag[100+TB];              // the tag to be compared 
      char name[100+TB];             // content name (filename)
      char contentType[100+TB];      // e.g. text/html
      char mimeType[100+TB];         // 
      char contentEncoding[100+TB];  // 
      int headerOnly;
      time_t modTime;
      time_t expireAt;
};

//***************************************************************************
// RGBA Stuff
//***************************************************************************

typedef unsigned char t_rgba[4];
typedef unsigned char* p_rgba;

enum RGB
{
   rgbR,
   rgbG,
   rgbB,
   rgbA
};

p_rgba str2rgba(const char* value, p_rgba rgba);
p_rgba int2rgba(int r, int g, int b, int a, p_rgba rgba);
void rgba2int(p_rgba rgba, int& r, int& g, int& b, int& a);

//***************************************************************************
// Wrapper Regual Expression Library
//***************************************************************************

enum Option
{
  repUseRegularExpression = 1,
  repIgnoreCase = 2
};

int rep(const char* string, const char* expression, Option options = repUseRegularExpression);

int rep(const char* string, const char* expression, 
        const char*& s_location, Option options = repUseRegularExpression);

int rep(const char* string, const char* expression, const char*& s_location, 
        const char*& e_location, Option options = repUseRegularExpression);


//***************************************************************************
// Log Duration
//***************************************************************************

class LogDuration
{
   public:

      LogDuration(const char* aMessage, int aLogLevel = 2);
      ~LogDuration();

      void show(const char* label = "");

   protected:

      char message[1000];
      uint64_t durationStart;
      int logLevel;
};

//***************************************************************************
// std::string trim stuff
//***************************************************************************

static inline std::string &ltrim(std::string &s) 
{
   s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
   return s;
}

static inline std::string &rtrim(std::string &s) 
{
   s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
   return s;
}

static inline std::string &trim(std::string &s) 
{
   return ltrim(rtrim(s));
}

//***************************************************************************
// Class Str
//***************************************************************************

class Str
{
   public:

      enum Case
      {
         cUpper,
         cLower
      };

      // Manipulation

      static char* rTrim(char* buf);
      static char* lTrim(char* buf);
      static char* allTrim(char* buf);
      static const char* toCase(Case cs, char* str);

      // converting

      static const char* toStr(const char* s);
      static const char* toStr(bool value);
      static const char* toStr(int value);
      static const char* toStr(double value, int precision = 2);

      // Checks

      static int isEmpty(const char* buf);
      static int isBlank(const char* buf);
      static const char* notNull(const char* s)   { return s ? s : ""; }
};

int clen(const char* s);
string replaceChar(string str, char ch1, char ch2);

//***************************************************************************
#endif //___COMMON_H
