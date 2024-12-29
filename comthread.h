//***************************************************************************
// Group VDR/GraphTFTng
// File comthread.h
// Date 31.10.06
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
// (c) 2006-2013 Jörg Wendel
//--------------------------------------------------------------------------
// Class ComThread
//***************************************************************************

#ifndef __GTFT_COMTHREAD_H__
#define __GTFT_COMTHREAD_H__

#include <vector>

#include <vdr/plugin.h>
#include <vdr/remote.h>
#include <tcpchannel.h>

#include <renderer.h>
#include <service.h>

using std::vector;

//***************************************************************************
// Communication Thread
//***************************************************************************

class ComThread : protected cThread, protected cRemote, public cGraphTftComService
{
   public:
      
      enum Misc
      {
         maxBuf = 512*1024
      };

      struct TcpClient
      {
         TcpChannel* channel;
         int jpgQuality;
         time_t lastCheck;
      };

      ComThread(void* aDisplay, int width, int height);
      virtual ~ComThread();

      void stop()                        { running = false; }
      int refresh();

      int init(Renderer* aRenderer, unsigned int aPort = 0, const char* aHost = 0);
      bool Start()                       { return cThread::Start(); }
      void Stop();

      void setHost(const char* aHost)    { strcpy(host, aHost); }
      void setPort(unsigned short aPort) { port = aPort; }
      void setJpegQuality(int value)     { jpgQuality = value; }

   protected:
      
      void Action();

      int close(TcpClient* client, int status, const char* message = 0);
      int read(fd_set* readSet);
      int read(TcpClient* client);
      int refresh(TcpClient* client);

      virtual bool Put(uint64_t Code, bool Repeat = false, bool Release = false);

      // data

      TcpChannel* listener;
      Renderer* renderer;
      void* display;
      cMutex _mutex;

      int themeWidth;
      int themeHeight;

      char* buffer;
      int bufferSize;
      int timeout;
      int checkTime;
      int running;
      unsigned short port;
      char host[100+TB];
      int pid;
      int jpgQuality;

      vector<TcpClient> clients;
};

//***************************************************************************
#endif // __GTFT_COMTHREAD_H__
