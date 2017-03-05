//***************************************************************************
// Group VDR/GraphTFT
// File graphtft.hpp
// Date 28.10.06 - Jörg Wendel
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
//--------------------------------------------------------------------------
// Class ComThread
// Class TcpChannel
//***************************************************************************

#ifndef __COMTHREAD_HPP__
#define __COMTHREAD_HPP__

#include <QObject>
#include <QThread>
#include <QReadWriteLock>

#define __FRONTEND
#include <../common.h>

#include <common.hpp>
#include <../service.h>

#include <unistd.h>

//***************************************************************************
// TcpChannel
//***************************************************************************

class TcpChannel
{
   public:

      // declarations

      enum Errors
      {
         errChannel = -100,
         
         errUnknownHostname,
         errBindAddressFailed,
         errAcceptFailed,
         errListenFailed,
         errConnectFailed,
         errIOError,
         errConnectionClosed,
         errInvalidEndpoint,
         errOpenEndpointFailed,

         // Warnungen

         wrnNoEventPending,
         errUnexpectedEvent,
         wrnChannelBlocked,
         wrnNoConnectIndication,
         wrnNoResponseFromServer,
         wrnNoDataAvaileble,
         wrnSysInterrupt,
         wrnTimeout
      };

      struct Header
      {
         int command;
         int size;
      };

      // object

      TcpChannel();
      ~TcpChannel();
      
      // api function

      int open(const char* aHost, int aPort);
      int close();
      int look(int aTimeout);
      int read(char* buf, int bufLen);
      int write(int command, const char* buf = 0, int bufLen = 0);
      int isConnected()    { return handle != 0; }

   private:

      int checkErrno();

      int handle;
      unsigned short port;
      char localHost[100];
      char remoteHost[100];
      long localAddr;
      long remoteAddr;
      long timeout;
      int lookAheadChar;
      int lookAhead;
      int nTtlReceived;
      int nTtlSent;
};

//***************************************************************************
// Communication Thread
//***************************************************************************

class ComThread : public QThread, public cGraphTftComService
{
   Q_OBJECT 

   public:
      
      enum Misc
      {
         maxBuffer = 1024*1024
      };

      ComThread();
      virtual ~ComThread();

      void stop()                        { running = false; }
      int mouseEvent(int x, int y, int button, int flag, int data = 0);
      int keyEvent(int key, int flag);

      const char* getBuffer()            { return buffer; }
      int getSize()                      { return header->size; }

      void setHost(const char* aHost)    { strcpy(host, aHost); }
      void setPort(unsigned short aPort) { port = aPort; }
      QReadWriteLock* getBufferLock()    { return &bufferLock; }

   signals:

      void updateImage(unsigned char* buffer, int size);

   protected:
      
      void update(unsigned char* buffer, int size);
      void run();
      int read();

      TcpChannel* line;

      char* buffer;
      int bufferSize;
      QReadWriteLock bufferLock;

      long timeout;
      int running;
      TcpChannel::Header* header;
      unsigned short port;
      char host[100];
};

//***************************************************************************
#endif  // __COMTHREAD_HPP__
