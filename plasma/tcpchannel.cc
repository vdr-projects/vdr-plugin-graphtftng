//***************************************************************************
// Group VDR/GraphTFT
// File tcpchannel.cc
// Date 28.10.06 - Jörg Wendel
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
//--------------------------------------------------------------------------
// Class TcpChannel
//***************************************************************************

#include <sys/socket.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include "comthread.hpp"

//***************************************************************************
// Class TcpChannel
//***************************************************************************
//***************************************************************************
// Object
//***************************************************************************

TcpChannel::TcpChannel()
{
   handle = 0;
   port = 0;
   *localHost = 0;
   *remoteHost = 0;
   localAddr = 0;
   remoteAddr = 0;
   timeout = 30;
   lookAheadChar = false;
   lookAhead = 0;
   nTtlSent = 0;
   nTtlReceived = 0;
}

TcpChannel::~TcpChannel()
{
   // nothing yet !
}

//***************************************************************************
// Open
//***************************************************************************

int TcpChannel::open(const char* aHost, int aPort)
{
   const char* hostName;
   struct sockaddr_in localSockAddr, remoteSockAddr;
   struct hostent* hostInfo;
   int aHandle;

   if (!aHost || !*aHost) 
      return -1;

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

   // none blocking

   if (fcntl(handle, F_SETFL, O_NONBLOCK) < 0)
      return fail;

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
   struct timeval timeout;
   fd_set readFD, writeFD, exceptFD;
   int n;

   // time-out for select

   timeout.tv_sec  = aTimeout;
   timeout.tv_usec = 1;

   // clear and set file-descriptors

   FD_ZERO(&readFD);
   FD_ZERO(&writeFD);
   FD_ZERO(&exceptFD);

   FD_SET(handle, &readFD);
   FD_SET(handle, &writeFD);
   FD_SET(handle, &exceptFD);

   // look event

   n = ::select(handle+1, &readFD, (aTimeout ? 0 : &writeFD) , &exceptFD, &timeout);

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
// Write to client
//***************************************************************************

int TcpChannel::write(int command, const char* buf, int bufLen)
{
   struct timeval wait;
   int result,  nfds;
   fd_set writeFD;
   int nSent = 0;
   Header header;

   //cMutexLock lock(&_mutex);

   if (buf && !bufLen)
      bufLen = strlen(buf);

   tell(eloDebug, "Writing (%d) header bytes, command (%d), size (%ld)", 
     sizeof(Header), command, bufLen);

   header.command = htonl(command);
   header.size = htonl(bufLen);
   result = ::write(handle, &header, sizeof(Header));

   if (result != sizeof(Header))
      return errIOError;

   if (!buf)
      return 0;

   tell(eloDebug, "Writing (%ld) kb now", bufLen/1024);

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
   if (!handle)
      return success;
   
   ::close(handle);
   handle = 0;

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
