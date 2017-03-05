//***************************************************************************
// Group VDR/GraphTFT
// File tcpchannel.cc
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
// (c) 2006-2014 J�rg Wendel
//--------------------------------------------------------------------------
// Class TcpChannel
//***************************************************************************

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>

#include "../common.h"
#include "tcpchannel.h"

//***************************************************************************
// Object
//***************************************************************************

TcpChannel::TcpChannel(int aTimeout, int aHandle)
{
   handle = aHandle;
   timeout = aTimeout;

   localAddr = 0;
   port = 0;
   remoteAddr = 0;

   *localHost = 0;
   *remoteHost = 0;

   nTtlSent = 0;
   nTtlReceived = 0;

   lookAheadChar = false;
   lookAhead = 0;
}

TcpChannel::~TcpChannel()
{
   close();
}

//***************************************************************************
// OpenLstn -> Start Listener
//***************************************************************************

int TcpChannel::openLstn(unsigned short aPort, const char* aLocalHost)
{
   struct sockaddr_in localSockAddr;
   struct hostent* hostInfo;
   int value = 1;
   int aHandle;

   // clear

   memset((char*)&localSockAddr, 0, sizeof(localSockAddr));

   // init

   localSockAddr.sin_family = AF_INET;

   // resolve local host

   if (aLocalHost && *aLocalHost)
   {
      // search alias

      if ((hostInfo = ::gethostbyname(aLocalHost)))
         memcpy((char*)&localAddr, hostInfo->h_addr, hostInfo->h_length);

      else if ((unsigned int)(localAddr = inet_addr(aLocalHost)) == INADDR_NONE)
      {
         tell(1, "unknown hostname '%s'", aLocalHost);
         return fail;
      }

      // set local endpoint

      memcpy(&localSockAddr.sin_addr, &localAddr, sizeof(struct in_addr));
   }

   // Server-Socket

   localSockAddr.sin_port = htons(aPort);

   // open socket

   if ((aHandle = ::socket(PF_INET, SOCK_STREAM, 0)) < 0)
   {
      tell(1, "Error: ");
      return fail;
   }

   // set socket non-blocking

   if (fcntl(aHandle, F_SETFL, O_NONBLOCK) < 0)
      tell(1, "Error: Setting socket options failed, errno (%d)", errno);

   setsockopt(aHandle, SOL_SOCKET, SO_REUSEADDR,
               (char*)&value, sizeof(value));

   // bind address to socket

   if (::bind(aHandle, (struct sockaddr*)&localSockAddr, sizeof(localSockAddr)) < 0)
   {
      ::close(aHandle);
      tell(1, "Error: Bind failed, errno (%d)", errno);

      return fail;
   }

   if (::listen(aHandle, 5) < 0)
   {
      ::close(aHandle);

      return fail;
   }

   // save

   handle = aHandle;
   port = aPort;

   return success;
}

//***************************************************************************
// Open
//***************************************************************************

int TcpChannel::open(unsigned short aPort, const char* aHost)
{
   const char* hostName;
   struct sockaddr_in localSockAddr, remoteSockAddr;
   struct hostent* hostInfo;
   int aHandle;

   if (!aHost || !*aHost) 
      return fail;

   hostName = aHost;

   // clear

   memset((char*)&localSockAddr, 0, sizeof(localSockAddr));
   memset((char*)&remoteSockAddr, 0, sizeof(remoteSockAddr));

   // init

   localSockAddr.sin_family = remoteSockAddr.sin_family = AF_INET;
   remoteSockAddr.sin_port = htons(aPort); 

   // resolve local host

   if (localHost && *localHost)
   {
      // search alias

      if ((hostInfo = ::gethostbyname(localHost)))
         memcpy((char*)&localAddr, hostInfo->h_addr, hostInfo->h_length);

      else if ((localAddr = inet_addr(localHost)) == (int)INADDR_NONE)
         return errUnknownHostname;

      // set local endpoint

      memcpy(&localSockAddr.sin_addr, &localAddr, sizeof(struct in_addr));
   }

   // map hostname to ip

   if ((hostInfo = ::gethostbyname(hostName)))
      memcpy((char*)&remoteAddr, hostInfo->h_addr, hostInfo->h_length);

   else if ((remoteAddr = inet_addr(hostName)) == (int)INADDR_NONE)
      return errUnknownHostname;

   // save hostname

   strncpy(remoteHost, hostName, sizeof(remoteHost));

   // set sockaddr

   memcpy(&remoteSockAddr.sin_addr, &remoteAddr, sizeof(struct in_addr));

   // create new socket

   if ((aHandle = socket(PF_INET, SOCK_STREAM, 0)) < 0)
      return errOpenEndpointFailed;

   // bind only if localSockAddr is set

   if (*((int*)&localSockAddr.sin_addr) != 0)
   {
      // bind local address to socket

      if (::bind(aHandle, (struct sockaddr*)&localSockAddr, sizeof(localSockAddr)) < 0)
      {
         ::close(aHandle);

         return errBindAddressFailed;
      }
   }

   // connect to server

   if (connect(aHandle, (struct sockaddr*)&remoteSockAddr, sizeof(remoteSockAddr)) < 0)
   {
      ::close(aHandle);

      if (errno != ECONNREFUSED)
         return errConnectFailed;

      return wrnNoResponseFromServer;
   }

   // save results

   handle = aHandle;
   port   = aPort;

   return success;
}

//***************************************************************************
// Read
//***************************************************************************

int TcpChannel::read(char* buf, int bufLen)
{
   int nfds, result;
   fd_set readFD;
   int nReceived;
   struct timeval wait;

   if (!handle)
      return fail;

   memset(buf, 0, bufLen);
   nReceived = 0;

   if (lookAhead)
   {
      *(buf) = lookAheadChar;
      lookAhead = false;
      nReceived++;
   }
   
   while (nReceived < bufLen)
   {
      result = ::read(handle, buf + nReceived, bufLen - nReceived);
      
      if (result < 0)
      {
         if (errno != EWOULDBLOCK)
            return checkErrno();
         
         // time-out for select
         
         wait.tv_sec  = timeout;
         wait.tv_usec = 0;
         
         // clear and set file-descriptors
         
         FD_ZERO(&readFD);
         FD_SET(handle, &readFD);
         
         // look event
         
         if ((nfds = ::select(handle+1, &readFD, 0, 0, &wait)) < 0)
            return checkErrno();

         // no event occured -> timeout
         
         if (nfds == 0)
            return wrnTimeout;
      }
      
      else if (result == 0)
      {
         // connection closed -> eof received

         return errConnectionClosed;
      }
      
      else
      {
         // inc read char count
         
         nReceived += result;
      }
   }

   nTtlReceived += nReceived;

   return success;
}

//***************************************************************************
// Look
//***************************************************************************

int TcpChannel::look(int aTimeout)
{
   struct timeval tv;
   fd_set readFD, writeFD, exceptFD;
   int n;

   if (!handle)
      return fail;

   // time-out for select

   tv.tv_sec  = aTimeout;
   tv.tv_usec = 1;

   // clear and set file-descriptors

   FD_ZERO(&readFD);
   FD_ZERO(&writeFD);
   FD_ZERO(&exceptFD);

   FD_SET(handle, &readFD);
   FD_SET(handle, &writeFD);
   FD_SET(handle, &exceptFD);

   // look event

   n = ::select(handle+1, &readFD, (aTimeout ? 0 : &writeFD), &exceptFD, &tv);

   if (n < 0)
      return checkErrno();

   // check exception

   if (FD_ISSET(handle, &exceptFD))
      return errUnexpectedEvent;

   // check write ok

   if (!FD_ISSET(handle, &writeFD))
      return wrnChannelBlocked;

   // check read-event

   if (!FD_ISSET(handle, &readFD))
      return wrnNoEventPending;

   // check first-char

   if (::read(handle, &lookAheadChar, 1) == 0)
      return errConnectionClosed;

   // look ahead char received

   lookAhead = true;

   return success;
}

//***************************************************************************
// Listen
//***************************************************************************

int TcpChannel::listen(TcpChannel*& child)
{
   struct sockaddr_in remote;
   struct timeval tv;
   fd_set readFD;
   int aHandle, num, len;

   child = 0;
   tv.tv_sec  = 0;
   tv.tv_usec = 1;
   len = sizeof(remote);

   // clear and set file-descriptor

   FD_ZERO(&readFD);
   FD_SET(handle, &readFD);

   // call select to look for request

   if ((num = ::select(handle+1, &readFD,(fd_set*)0,(fd_set*)0, &tv)) < 0)
      return checkErrno();

   if (!FD_ISSET(handle, &readFD))
      return wrnNoConnectIndication;

   // accept client

   if ((aHandle = ::accept(handle, (struct sockaddr*)&remote, (socklen_t*)&len)) < 0)
   {
      tell(1, "Error: Accept failed, errno was %d - '%s'", errno, strerror(errno));
      return errAcceptFailed;
   }

   // set none blocking, event for the new connection

   if (fcntl(aHandle, F_SETFL, O_NONBLOCK) < 0)
      return fail;

   // create new tcp channel

   child = new TcpChannel(timeout, aHandle);

   return success;
}

//***************************************************************************
// Write to client
//***************************************************************************

int TcpChannel::write(int command, const char* buf, int bufLen)
{
   struct timeval wait;
   int result,  nfds;
   fd_set writeFD;
   int nSent = 0;
   Header header;

   if (!handle)
      return fail;

#ifdef VDR_PLUGIN
   cMutexLock lock(&_mutex);
#endif

   if (buf && !bufLen)
      bufLen = strlen(buf);

   tell(eloDebug, "Writing (%ld) header bytes, command (%d), size (%d)", 
        sizeof(Header), command, bufLen);

   header.command = htonl(command);
   header.size = htonl(bufLen);
   result = ::write(handle, &header, sizeof(Header));

   if (result != sizeof(Header))
      return errIOError;

   if (!buf)
      return success;

   tell(eloDebug, "Writing (%d) kb now", bufLen/1024);

   do
   {
      result = ::write(handle, buf + nSent, bufLen - nSent);

      if (result < 0)
      {
         if (errno != EWOULDBLOCK)
            return checkErrno();
         
         // time-out for select

         wait.tv_sec  = timeout;
         wait.tv_usec = 0;

         // clear and set file-descriptors

         FD_ZERO(&writeFD);
         FD_SET(handle, &writeFD);

         // look event

         if ((nfds = ::select(handle+1, 0, &writeFD, 0, &wait)) < 0)
         {
            // Error: Select failed

            return checkErrno();
         }

         // no event occured -> timeout

         if (nfds == 0)
            return wrnTimeout;
      }
      else
      {
         nSent += result;
      }

   } while (nSent < bufLen);

   // increase send counter

   nTtlSent += nSent;

   return success;
}

//***************************************************************************
// Close
//***************************************************************************

int TcpChannel::close()
{
   if (handle)
   {
      ::close(handle);
      handle = 0;
   }

   return success;
}

//***************************************************************************
// Check Errno
//***************************************************************************

int TcpChannel::checkErrno()
{
   switch (errno)
   {
      case EINTR:       return wrnSysInterrupt;
      case EBADF:       return errInvalidEndpoint;
      case EWOULDBLOCK: return wrnNoDataAvaileble;
      case ECONNRESET:  return errConnectionClosed;
      default:          return errIOError;
   }
}
