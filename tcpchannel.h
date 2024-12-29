//***************************************************************************
// Group VDR/GraphTFT
// File tcpchannel.h
// Date 31.10.06
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
// (c) 2006-2008 Jörg Wendel
//--------------------------------------------------------------------------
// Class TcpChannel
//***************************************************************************

#ifndef __GTFT_TCPCHANNEL_H__
#define __GTFT_TCPCHANNEL_H__

#include <vdr/thread.h>

//***************************************************************************
// Class TcpChannel
//***************************************************************************

class TcpChannel
{
   public:	

     enum Errors
      {
         errChannel = -100,

         errUnknownHostname,      // 99
         errBindAddressFailed,    // 98
         errAcceptFailed,
         errListenFailed,
         errConnectFailed,        // 95
         errIOError,              // 94
         errConnectionClosed,     // 93
         errInvalidEndpoint,
         errOpenEndpointFailed,   // 91

         // Warnungen

         wrnNoEventPending,       // 90
         errUnexpectedEvent,      // 89
         wrnChannelBlocked,       // 88
         wrnNoConnectIndication,  // 87
         wrnNoResponseFromServer, // 86
         wrnNoDataAvaileble,      // 85
         wrnSysInterrupt,         // 84
         wrnTimeout               // 83
      };

#pragma pack(1)
      struct Header
      {
         int command;
         int size;
      };
#pragma pack()

		TcpChannel(int aTimeout = 2, int aHandle = 0);
		~TcpChannel();

      int open(unsigned short aPort, const char* aLocalHost = 0);
      int close();
		int listen(TcpChannel*& child);
      int look(int aTimeout = 0);
      int read(char* buf, int bufLen);
      int write(int command, const char* buf = 0, int bufLen = 0);

      int isConnected()    { return handle != 0; }
      int getHandle()      { return handle; }

   private:

      int checkErrno();

      // data

      int lookAheadChar;
      int lookAhead;

      int handle;
      unsigned short port;   
      long localAddr;
      long nTtlSent;
      long nTtlReceived;
      long timeout;        // for read/write

      cMutex _mutex;
};

//***************************************************************************
#endif // __GTFT_TCPCHANNEL_H__
