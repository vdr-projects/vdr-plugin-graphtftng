//***************************************************************************
// Group VDR/GraphTFT
// File tcpchannel.h
// Date 31.10.06
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
// (c) 2006-2014 Jörg Wendel
//--------------------------------------------------------------------------
// Class TcpChannel
//***************************************************************************

#ifndef __GTFT_TCPCHANNEL_H__
#define __GTFT_TCPCHANNEL_H__

//***************************************************************************
// Class TcpChannel
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

#pragma pack(1)
      struct Header
      {
         int command;
         int size;
      };
#pragma pack()

      // object

      TcpChannel(int aTimeout = 2, int aHandle = 0);
      ~TcpChannel();
      
      // api function

      int openLstn(unsigned short aPort, const char* aLocalHost = 0);
      int open(unsigned short aPort, const char* aHost);
      int close();
		int listen(TcpChannel*& child);
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

#ifdef VDR_PLUGIN
      cMutex _mutex;
#endif
};

//***************************************************************************
#endif // __GTFT_TCPCHANNEL_H__
