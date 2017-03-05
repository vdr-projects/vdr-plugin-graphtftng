//***************************************************************************
// Group VDR/GraphTFT
// File comthread.cc
// Date 28.10.06 - Jörg Wendel
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
//--------------------------------------------------------------------------
// Class ComThread
//***************************************************************************

#include <arpa/inet.h>

#include "tcpchannel.h"
#include "graphtft.hpp"

//***************************************************************************
// Object
//***************************************************************************

ComThread::ComThread()
   : cMyThread()
{
   line = new TcpChannel();

   bufferSize = maxBuffer;

   buffer = new char[bufferSize+1];
   header = new TcpChannel::Header;

   timeout = 1;
   port = -1;
   *host = 0;
   client = 0;
   jpegQuality = na;
}

ComThread::~ComThread()
{
   if (line->isConnected())
   {
      tell(eloAlways, "Logout from server, closing tcp connection");
      line->write(cGraphTftComService::cmdLogout);
      line->close();
   }

   delete line;
   delete header;
   delete[] buffer;
}

void ComThread::stop()
{ 
   running = false; 

   Cancel(2);
}

//***************************************************************************
// Run
//***************************************************************************

void ComThread::Action()
{
   const int checkTimeout = 30;

   int status;
   time_t lastCheck = time(0);
   int quality = htonl(jpegQuality);

   running = true;

   while (running)
   {
      if (!line->isConnected())
      {
         tell(eloAlways, "Trying connecting to '%s' at port (%d)", host, port);

         if (line->open(port, host) == 0)
         {
            tell(eloAlways, "Connection to '%s' established", host);

            if (jpegQuality > na)
               line->write(cGraphTftComService::cmdJpegQuality, (char*)&quality, sizeof(int));
         }
         else
            tell(eloAlways, "Connecting to '%s' failed", host);
      }

      while (line->isConnected() && running)
      {
         if (lastCheck+checkTimeout < time(0))
         {
            line->write(cGraphTftComService::cmdCheck);
            lastCheck = time(0);
         }

         if ((status = line->look(1)) != success)
         {
            if (status != TcpChannel::wrnNoEventPending)
            {
               tell(eloAlways, "Error: Communication problems, closing line! status was (%d)", 
                    status);
               line->close();

               break;
            }
            
            continue;
         }
         
         if ((status = read()) != 0)
         {
            line->close();
            tell(eloAlways, "Error: Communication problems, closing line! status was (%d)", 
                 status);
         }
      }

      if (!running) break;

      tell(eloAlways, "Retrying in %ld seconds", timeout);

      for (int i = 0; i < timeout && running; i++)
         sleep(1);
   }
}

//***************************************************************************
// Transmit events
//***************************************************************************

int ComThread::mouseEvent(int x, int y, int button, int flag, int data)
{
   GraphTftTouchEvent m;

   m.x = htonl(x);
   m.y = htonl(y);
   m.button = htonl(button);
   m.flag = htonl(flag);
   m.data = htonl(data);

   line->write(cGraphTftComService::cmdMouseEvent, (char*)&m, sizeof(GraphTftTouchEvent));

   return 0;
}

//***************************************************************************
// ...
//***************************************************************************

int ComThread::keyEvent(int key, int flag)
{
   GraphTftTouchEvent m;

   m.x = htonl(0);
   m.y = htonl(0);
   m.button = htonl(key);
   m.flag = htonl(flag | efKeyboard);

   line->write(cGraphTftComService::cmdMouseEvent, (char*)&m, sizeof(GraphTftTouchEvent));

   return 0;
}

//***************************************************************************
// Read
//***************************************************************************

int ComThread::read()
{
   int status;
   TcpChannel::Header tmp;

   // es stehen Daten an, erst einmal den Header abholen ..

   if ((status = line->read((char*)&tmp, sizeof(TcpChannel::Header))) == 0)
   {
      header->command = ntohl(tmp.command);
      header->size = ntohl(tmp.size);

      switch (header->command)
      {
         case cGraphTftComService::cmdWelcome:
         {
            tell(eloAlways, "Got welcome");

            break;
         }

         case cGraphTftComService::cmdLogout:
         {
            tell(eloAlways, "Got logout from client, closing line");
            line->close();

            break;
         }

         case cGraphTftComService::cmdData:
         {
            tell(eloDebug, "Debug: Start reading %d kb from TCP", header->size/1024);
            status = line->read(buffer, header->size);
            tell(eloDebug, "Debug: Received %d kb", header->size/1024);
            
            if (status == 0 && client) 
               client->updateImage((unsigned char*)buffer, header->size);
            
            break;
         }

         case cGraphTftComService::cmdMouseEvent:
         {
            GraphTftTouchEvent ev;

            status = line->read((char*)&ev, header->size);
            tell(eloAlways, "Got mouse event, button (%d) at (%d/%d)", ev.button, ev.x, ev.y); 

            break;
         }

         default:
         {
            tell(eloAlways, "Got unexpected protocol (%d), aborting", header->command); 
            status = -1;

            break;
         }
      }
   }   

   return status;
}
