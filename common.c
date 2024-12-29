/**
 *  GraphTFT plugin for the Video Disk Recorder 
 * 
 *  common.c - A plugin for the Video Disk Recorder
 *
 *  (c) 2004 Lars Tegeler, Sascha Volkenandt
 *  (c) 2006-2008 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 * $Id: common.c,v 1.10 2007/12/03 19:58:20 root Exp $
 *
 **/

// includes

#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <syslog.h>
#include <regex.h>
#include <string.h>
#include <errno.h>

#include <string>

using std::string;

#ifdef VDR_PLUGIN
# include "setup.h"
# include <vdr/tools.h>
# include <vdr/thread.h>

  cMutex logMutex;
#endif

#include "common.h"

int logLevel = eloOff;
int logDevice = devSyslog;

//***************************************************************************
// Log
//***************************************************************************

void tell(int eloquence, const char* format, ...)
{
   if (logLevel < eloquence)
      return ;

   const int sizeBuffer = 100000;
   char t[sizeBuffer+100];
   va_list ap;
   time_t  now;
   struct tm tim;

   memset(&tim, 0, sizeof(tm));

#ifdef VDR_PLUGIN
   cMutexLock lock(&logMutex);
#endif
   
   va_start(ap, format);

   switch (logDevice)
   {
      case devNone: break;

      case devStdOut:
      {
         char buf[50+TB];
         timeval tp;
         
         gettimeofday(&tp, 0);
         tm* tm = localtime(&tp.tv_sec);

         sprintf(buf,"%2.2d:%2.2d:%2.2d,%3.3ld ",
                 tm->tm_hour, tm->tm_min, tm->tm_sec, 
                 tp.tv_usec / 1000);

         vsnprintf(t, sizeBuffer, format, ap);         
         printf("%s %s\n", buf, t);

         break;
      }

      case devSyslog:
      {
         snprintf(t, sizeBuffer, "[graphTFT] ");
         vsnprintf(t+strlen(t), sizeBuffer-strlen(t), format, ap);

         syslog(LOG_DEBUG, "%s", t);

         break;
      }

      case devFile:
      {
         FILE* lf;
         
         lf = fopen("/tmp/graphTFT.log", "a");

         if (lf) 
         {
            timeval tp;
            tm* tm;

            vsnprintf(t + 24, sizeBuffer+21, format, ap);

            time(&now);
            strftime(t, sizeof(t), "%Y.%m.%d ", localtime_r(&now, &tim));
            gettimeofday(&tp, 0);
            tm = localtime_r(&tp.tv_sec, &tim);
           
            sprintf(t + strlen(t),"%2.2d:%2.2d:%2.2d,%3.3ld",
                    tm->tm_hour, tm->tm_min, tm->tm_sec, tp.tv_usec / 1000);

            t[23] = ' ';           
            fprintf(lf, "%s\n", t);
            fclose(lf);
         }
            
         break;
      }
   }

   va_end(ap);
}

//***************************************************************************
// Save Realloc
//***************************************************************************

char* srealloc(void* ptr, size_t size)
{
   void* n = realloc(ptr, size);

   if (!n)
   {
      free(ptr);
      ptr = 0;
   }

   return (char*)n;
}

//***************************************************************************
// RGBA Stuff
//***************************************************************************

p_rgba str2rgba(const char* value, p_rgba rgba)
{
   int index;
   char* pc;
   char* col = strdup(value);

   memset(rgba, 255, sizeof(t_rgba));

   // "220:30:110:200" -> rgba

   for (index = 0, pc = strtok(col, ":"); pc && index < 4; pc = strtok(0, ":"), index++)
      rgba[index] = (unsigned char)atoi(pc);

   free(col);

   return rgba;
}

p_rgba int2rgba(int r, int g, int b, int a, p_rgba rgba)
{
   rgba[rgbR] = r;
   rgba[rgbG] = g;
   rgba[rgbB] = b;
   rgba[rgbA] = a;

   return rgba;
}

void rgba2int(p_rgba rgba, int& r, int& g, int& b, int& a)
{
   r = rgba[rgbR];
   g = rgba[rgbG];
   b = rgba[rgbB];
   a = rgba[rgbA];
}

//***************************************************************************
// check if a file exists
//***************************************************************************

int fileExists(std::string filename) 
{
   struct stat file_stat;

   return (stat(filename.c_str(), &file_stat) == 0) ? true : false;
}

const char* suffixOf(const char* path)
{
   const char* p;

   if (path && (p = strrchr(path, '.')))
      return p+1;

   return "";
}

#ifdef VDR_PLUGIN

//***************************************************************************
// To UTF8
//***************************************************************************

int toUTF8(char* out, int outMax, const char* in)
{
   iconv_t cd;
   size_t ret;
   char* toPtr;
   char* fromPtr;
   size_t fromLen, outlen;

   const char* to_code = "UTF-8";
   const char* from_code = "ISO8859-1";

   if (!out || !in || !outMax)
      return fail;

   *out = 0;
   fromLen = strlen(in);
   
   if (!fromLen)
      return fail;
   
#if VDRVERSNUM >= 10509
   switch (I18nCurrentLanguage())
#else
   switch (Setup.OSDLanguage) 
#endif
   {
      case 11: from_code = "ISO8859-7";  break;
      case 13: 
      case 17: from_code = "ISO8859-2";  break;
      case 16: from_code = "ISO8859-5";  break;
      case 18: from_code = "ISO8859-15"; break;
      default: from_code = "ISO8859-1";  break;
	}

	cd = iconv_open(to_code, from_code);

   if (cd == (iconv_t)-1) 
      return fail;

   fromPtr = (char*)in;
   toPtr = out;
   outlen = outMax;

   ret = iconv(cd, &fromPtr, &fromLen, &toPtr, &outlen);

   *toPtr = 0;   
   iconv_close(cd);

   if (ret == (size_t)-1)
   {
      tell(0, "Converting [%s] from '%s' to '%s' failed", 
           fromPtr, from_code, to_code);

		return fail;
   }

   return success;
}
#endif

//***************************************************************************
// Load From File
//***************************************************************************

int loadFromFile(const char* infile, MemoryStruct* data)
{
   FILE* fin;
   struct stat sb;

   data->clear();

   if (!fileExists(infile))
   {
      tell(0, "File '%s' not found'", infile);
      return fail;
   }

   if (stat(infile, &sb) < 0)
   {
      tell(0, "Can't get info of '%s', error was '%s'", infile, strerror(errno));
      return fail;
   }

   if ((fin = fopen(infile, "r")))
   {
      const char* sfx = suffixOf(infile);

      data->size = sb.st_size;
      data->modTime = sb.st_mtime;
      data->memory = (char*)malloc(data->size);
      fread(data->memory, sizeof(char), data->size, fin);
      fclose(fin);
      sprintf(data->tag, "%ld", (long int)data->size);

      if (strcmp(sfx, "gz") == 0)
         sprintf(data->contentEncoding, "gzip");
      
      if (strcmp(sfx, "js") == 0)
         sprintf(data->contentType, "application/javascript");

      else if (strcmp(sfx, "png") == 0 || strcmp(sfx, "jpg") == 0 || strcmp(sfx, "gif") == 0)
         sprintf(data->contentType, "image/%s", sfx);

      else if (strcmp(sfx, "ico") == 0)
         strcpy(data->contentType, "image/x-icon");

      else
         sprintf(data->contentType, "text/%s", sfx);
   }
   else
   {
      tell(0, "Error, can't open '%s' for reading, error was '%s'", infile, strerror(errno));
      return fail;
   }

   return success;
}

//***************************************************************************
// JPEG Dimensions
//***************************************************************************

int jpegDimensions(const char* path, unsigned int& pWidth, unsigned int& pHeight)
{
   MemoryStruct data;
   unsigned char* pData;

   pWidth = pHeight = 0;

   if (loadFromFile(path, &data) != success)
   {
      tell(0, "Error loading '%s', error was '%s'", path, strerror(errno));
      return fail;
   }

   pData = (unsigned char*)data.memory;

   if (pData[0] != 0xFF || pData[1] != 0xD8)
      return fail;
  
   // retrieve the block length of the first block since the first block will not contain the size of file

   for (unsigned int i = 4; i < data.size; i += 2)
   {
      unsigned short block_length = pData[i] * 256 + pData[i+1];   

      i += block_length;  // next block
      
      if (i >= data.size || pData[i] != 0xFF) 
         return fail;
      
      // 0xFFC0 is the "Start Of Frame" marker which contains the file size

      if (pData[i+1] == 0xC0)
      {
         pHeight = pData[i+5] * 256 + pData[i+6];
         pWidth = pData[i+7] * 256 + pData[i+8];
         
         return success;
      }
   }
   
   return fail;
}

//***************************************************************************
// Left Trim
//***************************************************************************

char* Str::lTrim(char* buf)
{
   if (buf)
   {
      char *tp = buf;

      while (*tp && strchr("\n\r\t ",*tp)) 
         tp++;

      memmove(buf, tp, strlen(tp) +1);
   }
   
   return buf;
}

//*************************************************************************
// Right Trim
//*************************************************************************

char* Str::rTrim(char* buf)
{
   if (buf)
   {
      char *tp = buf + strlen(buf);

      while (tp >= buf && strchr("\n\r\t ",*tp)) 
         tp--;

      *(tp+1) = 0;
   }
   
   return buf;
}

//*************************************************************************
// All Trim
//*************************************************************************

char* Str::allTrim(char* buf)
{
   return lTrim(rTrim(buf));
}

//*************************************************************************
// Is Empyt
//*************************************************************************

int Str::isEmpty(const char* buf)
{
   if (buf && *buf)
      return no;

   return yes;
}

int Str::isBlank(const char* buf)
{
   int i = 0;

   while (buf[i])
   {
      if (buf[i] != ' ' && buf[i] != '\t')
         return no;

      i++;
   }

   return yes;
}

const char* Str::toStr(const char* s)
{
   static char* buf = 0;
   static unsigned int sizeBuf = 0;

   if (!s)
      return "";

   if (!buf || sizeBuf < strlen(s))
   {
      if (buf) free(buf);

      sizeBuf = strlen(s);
      buf = (char*)malloc(sizeBuf);
   }

   strcpy(buf, s);

   return buf;
}

const char* Str::toStr(bool value)
{
   static char buf[10+TB];

   sprintf(buf, "%s", value ? "1" : "0");

   return buf;
}

const char* Str::toStr(int value)
{
   static char buf[100+TB];

   sprintf(buf, "%d", value);

   return buf;
}

const char* Str::toStr(double value, int precision)
{
   static char buf[100+TB];

   sprintf(buf, "%.*f", precision, value);

   return buf;
}

//***************************************************************************
// To Case (UTF-8 save)
//***************************************************************************

const char* Str::toCase(Case cs, char* str)
{
   char* s = str;
   int lenSrc = strlen(str);

   int csSrc;  // size of character

   for (int ps = 0; ps < lenSrc; ps += csSrc)
   {
      csSrc = std::max(mblen(&s[ps], lenSrc-ps), 1);
      
      if (csSrc == 1)
         s[ps] = cs == cUpper ? toupper(s[ps]) : tolower(s[ps]);
      else if (csSrc == 2 && s[ps] == (char)0xc3 && s[ps+1] >= (char)0xa0)
      {
         s[ps] = s[ps];
         s[ps+1] = cs == cUpper ? toupper(s[ps+1]) : tolower(s[ps+1]);
      }
      else
      {
         for (int i = 0; i < csSrc; i++)
            s[ps+i] = s[ps+i];
      }
   }

   return str;
}

//***************************************************************************
// Class LogDuration
//***************************************************************************

#ifdef VDR_PLUGIN

# include <vdr/plugin.h>

LogDuration::LogDuration(const char* aMessage, int aLogLevel)
{
   logLevel = aLogLevel;
   strcpy(message, aMessage);
   
   // at last !

   durationStart = cTimeMs::Now();
}

LogDuration::~LogDuration()
{
   tell(logLevel, "duration '%s' was (%ldms)",
     message, cTimeMs::Now() - durationStart);
}

void LogDuration::show(const char* label)
{
   tell(logLevel, "elapsed '%s' at '%s' was (%ldms)",
     message, label, cTimeMs::Now() - durationStart);
}

#endif

//**************************************************************************
//  Regular Expression Searching
//**************************************************************************

int rep(const char* string, const char* expression, Option options)
{
  const char* tmpA;
  const char* tmpB;

  return rep(string, expression, tmpA, tmpB, options);
}

int rep(const char* string, const char* expression, const char*& s_location, 
        Option options)
{
  const char* tmpA;

  return rep(string, expression, s_location, tmpA, options);
}


int rep(const char* string, const char* expression, const char*& s_location, 
        const char*& e_location, Option options)
{
   regex_t reg;
   regmatch_t rm;
   int status;
   int opt = 0;

   // Vorbereiten von reg fuer die Expressionsuche mit regexec
   // Flags:  REG_EXTENDED = Use Extended Regular Expressions
   //         REG_ICASE    = Ignore case in match.

   reg.re_nsub = 0;

   // Options umwandeln
   if (options & repUseRegularExpression)
     opt = opt | REG_EXTENDED;
   if (options & repIgnoreCase)
     opt = opt | REG_ICASE;
 
   if (regcomp( &reg, expression, opt) != 0)
     return fail;  

   // Suchen des ersten Vorkommens von reg in string

   status = regexec(&reg, string, 1, &rm, 0);
   regfree(&reg);

   if (status != 0) 
     return fail; 

   // Suche erfolgreich =>
   // Setzen der ermittelten Start- und Endpositionen

   s_location = (char*)(string + rm.rm_so);
   e_location = (char*)(string + rm.rm_eo);

   return success; 
}

#ifdef VDR_PLUGIN
//***************************************************************************
// STR / SNR
//***************************************************************************

#define FRONTEND_DEVICE "/dev/dvb/adapter%d/frontend%d"

int getFrontendSTR()
{
   uint16_t value = 0;
   cString dev = cString::sprintf(FRONTEND_DEVICE, cDevice::ActualDevice()->CardIndex(), 0);
   
   int fe = open(dev, O_RDONLY | O_NONBLOCK);

   if (fe < 0)
      return 0;

   CHECK(ioctl(fe, FE_READ_SIGNAL_STRENGTH, &value));
   close(fe);
   
   return value / 655;
}

int getFrontendSNR()
{
   uint16_t value = 0;
   cString dev = cString::sprintf(FRONTEND_DEVICE, cDevice::ActualDevice()->CardIndex(), 0);
   
   int fe = open(dev, O_RDONLY | O_NONBLOCK);

   if (fe < 0)
      return 0;

   CHECK(ioctl(fe, FE_READ_SNR, &value));
   close(fe);
   
   return value / 655;
}
#endif

uint64_t msNow()
{
  struct timeval t;

  if (gettimeofday(&t, NULL) == 0)
     return (uint64_t(t.tv_sec)) * 1000 + t.tv_usec / 1000;

  return 0;
}

string replaceChar(string str, char ch1, char ch2) 
{
   for (unsigned int i = 0; i < str.length(); ++i) 
   {
      if (str[i] == ch1)
         str[i] = ch2;
   }

  return str;
}

int clen(const char* s)
{
   int blen = strlen(s);
   int cs;
   int len = 0;
   
   for (int bp = 0; bp < blen; bp += cs, len++)
      cs = std::max(mblen(&s[bp], blen-bp), 1);
   
   return len;
}
