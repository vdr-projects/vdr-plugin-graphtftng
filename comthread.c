//***************************************************************************
// Group VDR/GraphTFT
// File comthread.c
// Date 28.10.06
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
// (c) 2006-2013 Jörg Wendel
//--------------------------------------------------------------------------
// Class ComThread
//***************************************************************************

#include "common.h"
#include "comthread.h"
#include "display.h"

//***************************************************************************
// Object
//***************************************************************************

ComThread::ComThread(void* aDisplay, int width, int height)
   : cRemote("graphtft-fe")
{
   display = aDisplay;
   themeWidth = width;
   themeHeight = height;

   bufferSize = maxBuf;

   buffer = new char[bufferSize+TB];

   running = false;
   timeout = 5;
   checkTime = 60;
   port = na;
   *host = 0;
   pid = 0;
   jpgQuality = 60;
   width = 720;
   height = 576;

   listener = new TcpChannel(timeout);
}

ComThread::~ComThread()
{
   if (display)
      ((cGraphTFTDisplay*)display)->clearComThread();

   Stop();

   // close all client connections

   while (clients.size())
   {
      clients[0].channel->close();
      delete clients[0].channel;
      clients.erase(clients.begin());
   }

   listener->close();

   delete listener;
   delete[] buffer;
}

//***************************************************************************
// Init
//***************************************************************************

int ComThread::init(Renderer* aRenderer, unsigned int aPort, const char* aHost)
{
   renderer = aRenderer;

   if (aHost)
      setHost(aHost);

   if (aPort)
      setPort(aPort);

   if (listener->open(port) != 0)
   {
      tell(0, "Error: Can't establish listener");
      return fail;
   }

   tell(0, "Listener established!");

   return success;
}

int ComThread::close(TcpClient* client, int status, const char* message)
{
   cMutexLock lock(&_mutex);

   vector<TcpClient>::iterator it;

   for (it = clients.begin(); it < clients.end(); it++)
   {
      TcpClient* c = &(*it);

      if (c == client)
      {
         if (message)
            tell(0, "(%d) %s", status, message);

         c->channel->close();
         delete c->channel;
         c->channel = 0;
         clients.erase(it);

         break;
      }
   }

   return done;
}

//***************************************************************************
// Stop
//***************************************************************************

void ComThread::Stop()
{
   if (running) 
   {
      isyslog("GraphTFT plugin try to stop communication thread");
      running = false;
      Cancel(3);
   }
}

//***************************************************************************
// Run
//***************************************************************************

void ComThread::Action()
{
   TcpClient client;
   struct timeval tv;
   fd_set readSet;
   int maxFD;                               // the highest-numbered descriptor
   int status;

   pid = getpid();
   tell(0, "TCP communication thread started (pid=%d)", pid);

   running = true;

   while (running)
   {
      if (!(time(0) % 120))
         tell(2, "still running, %ld clients connected", clients.size());

      // check client connections
      //    perform select on all connections
      
      FD_ZERO(&readSet);
      FD_SET(listener->getHandle(), &readSet);  // file descriptor of listener
      maxFD = listener->getHandle();
      
      _mutex.Lock();

      for (unsigned int i = 0; i < clients.size(); i++)
      {
         FD_SET(clients[i].channel->getHandle(), &readSet);
         maxFD = std::max(maxFD, clients[i].channel->getHandle());
      }

      _mutex.Unlock();
      
      tv.tv_sec  = 1;
      tv.tv_usec = 0;
      
      // call select and wait up to 1 second
      
      if ((status = select(maxFD+1, &readSet, 0, 0, &tv)) < 0)
      {
         tell(0, "Error: Select failed, error was %s", strerror(errno));
         continue;
      }
         
      if (status == 0)
         continue;                                          // no data pending
      
      // at least one tcp message pending
      
      read(&readSet);

      // check for new connection

      if (FD_ISSET(listener->getHandle(), &readSet))
      {
         FD_CLR(listener->getHandle(), &readSet);

         if (listener->listen(client.channel) == success)
         {
            tell(0, "Client connection accepted, now "
                 "%ld clients connected", clients.size()+1);
            
            client.lastCheck = time(0);
            client.jpgQuality = 30;   // speed up first draw on slow connections
            client.channel->write(cGraphTftComService::cmdWelcome);
            
            // initial refresh         
            
            if (refresh(&client) == success)
            {
               client.jpgQuality = jpgQuality;
               clients.push_back(client);
            }
            else
               close(&client, 0, "initial refresh failed");
         }
      }
   }

   isyslog("GraphTFT plugin tcp communication thread ended (pid=%d)", pid);
}

//***************************************************************************
// read
//***************************************************************************

int ComThread::read(fd_set* readSet)
{
   cMutexLock lock(&_mutex);
   int status;

   for (unsigned int i = 0; i < clients.size(); i++)
   {
      if (FD_ISSET(clients[i].channel->getHandle(), readSet))
      {
         FD_CLR(clients[i].channel->getHandle(), readSet);
         
         if ((status = read(&clients[i])) != success && status != TcpChannel::wrnTimeout)
         {
            close(&clients[i], status, "Error: Communication problems, read failed, closing line!");
            continue;
         }
      }

      if (time(0) > clients[i].lastCheck + checkTime)
         close(&clients[i], 0, "Error: Missing check command on tcp connection, closing line!");
   }

   return done;
}

//***************************************************************************
// Read
//***************************************************************************

int ComThread::read(TcpClient* client)
{
   int status;
   TcpChannel::Header tmp;
   TcpChannel::Header header;

   // es stehen Daten an, erst einmal den Header abholen ..

   if ((status = client->channel->read((char*)&tmp, sizeof(TcpChannel::Header))) == 0)
   {
      header.command = ntohl(tmp.command);
      header.size = ntohl(tmp.size);

      tell(3, "Got command %d with %d data bytes", header.command, header.size);

      switch (header.command)
      {
         case cGraphTftComService::cmdWelcome:
         {
            tell(1, "Got welcome");
            break;
         }

         case cGraphTftComService::cmdLogout:
         {
            tell(1, "Got logout from client, closing line");
            close(client, 0, "Closing connection due to client logout");

            break;
         }

         case cGraphTftComService::cmdData:
         {
            tell(7, "Got data");
            status = client->channel->read(buffer, header.size);
            break;
         }

         case cGraphTftComService::cmdJpegQuality:
         {
            int quality;

            status = client->channel->read((char*)&quality, header.size);
            quality = ntohl(quality);

            tell(0, "Got JPEG quality (%d)", quality);

            if (quality > 0 && quality <= 100)
               client->jpgQuality = quality;

            break;
         }

         case cGraphTftComService::cmdMouseEvent:
         {
            GraphTftTouchEvent ev;

            status = client->channel->read((char*)&ev, header.size);
           
            ev.x = ntohl(ev.x);
            ev.y = ntohl(ev.y);
            ev.button = ntohl(ev.button);
            ev.flag = ntohl(ev.flag);
            ev.data = ntohl(ev.data);

            tell(0, "Got mouse event, button (%d/%d) at (%d/%d)", ev.button, ev.flag, ev.x, ev.y); 

            if (ev.flag & ComThread::efKeyboard)
               Put(ev.button);
            else
               ((cGraphTFTDisplay*)display)->mouseEvent(ev.x, ev.y, ev.button, 
                                                        ev.flag, ev.data);

            break;
         }

         case cGraphTftComService::cmdStartCalibration:
         {
            ((cGraphTFTDisplay*)display)->setCalibrate(true);

            break;
         }

         case cGraphTftComService::cmdStopCalibration:
         {
            ((cGraphTFTDisplay*)display)->setCalibrate(false);

            break;
         }

         case cGraphTftComService::cmdCheck:
         {
            client->lastCheck = time(0);

            break;
         }

         default:
         {
            tell(0, "Got unexpected protocol (%d/%d), aborting", 
                 header.command, header.size); 
            status = fail;

            break;
         }
      }
   }
   
   return status;
}

//***************************************************************************
// Put Key Code
//***************************************************************************

bool ComThread::Put(uint64_t Code, bool Repeat, bool Release)
{ 
   tell(5, "Put key action (%ld)", Code);

   return cRemote::Put(Code, Repeat, Release); 
}

//***************************************************************************
// Refresh
//***************************************************************************

int ComThread::refresh()
{
   cMutexLock lock(&_mutex);
   int status;

   for (unsigned int i = 0; i < clients.size(); i++)
   {
      if ((status = refresh(&clients[i])) != success)
         close(&clients[i], status, "Refresh failed, closing connection");
   }

   return done;
}

int ComThread::refresh(TcpClient* client)
{
   long size;
   unsigned char* jpeg = 0;
   int status = success;

   if (!client->channel || !client->channel->isConnected())
      return fail;

   LogDuration ld("ComThread::refresh()", 2);
   
   if ((size = renderer->toJpeg(jpeg, client->jpgQuality)) > 0)
   {
      tell(7, "Info: Write %ld kb to %d", size/1024, client->channel->getHandle());
      
      status = client->channel->write(cGraphTftComService::cmdData, (char*)jpeg, size);

      free(jpeg);
   }

   return status;
}
