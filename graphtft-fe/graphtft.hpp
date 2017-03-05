//***************************************************************************
// Group VDR/GraphTFT
// File graphtft.hpp
// Date 28.10.06 - Jörg Wendel
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
//--------------------------------------------------------------------------
// Class GraphTft
// Class ComThread
//***************************************************************************

#ifndef __GRAPHTFT_HPP__
#define __GRAPHTFT_HPP__

#include <X11/Xlib.h>
#include <Imlib2.h>
#include <string.h>
#include <unistd.h>

#define __FRONTEND

#include "../common.h"
#include "../service.h"

#include "thread.h"
#include "tcpchannel.h"

class GraphTft;

//***************************************************************************
// Communication Thread
//***************************************************************************

class ComThread : public cMyThread, public cGraphTftComService
{
   public:
      
      enum Misc
      {
         maxBuffer = 1024*1024
      };

      ComThread();
      virtual ~ComThread();

      void stop();

      int mouseEvent(int x, int y, int button, int flag, int data = 0);
      int keyEvent(int key, int flag);

      const char* getBuffer()            { return buffer; }
      int getSize()                      { return header->size; }

      void setHost(const char* aHost)    { strcpy(host, aHost); }
      void setPort(unsigned short aPort) { port = aPort; }
      void setClient(GraphTft* aClient)  { client = aClient; }
      void setJpegQuality(int quality)   { jpegQuality = quality; }

   protected:
      
      void Action();
      int read();

      TcpChannel* line;

      char* buffer;
      int bufferSize;
      GraphTft* client;

      long timeout;
      int running;
      int jpegQuality;
      TcpChannel::Header* header;
      unsigned short port;
      char host[100];
};

//***************************************************************************
// Graph TFT
//***************************************************************************

class GraphTft
{
   public:
      
      GraphTft();
      virtual ~GraphTft();

      int init();
      int exit();
      int start();
      int run();
      int paint();

      void setArgs(int argc, char *argv[]);
      int sendEvent();
      void updateImage(const unsigned char* buffer, int size);
      void dumpImage(Imlib_Image image);
      void showUsage();
      int onMotion();
      int onButtonPress(XEvent event, int press);
      int onKeyPress(XEvent event);

      static void setEloquence(int aElo) { eloquence = aElo; }
      static int getEloquence()          { return eloquence; }

   protected:

      // functions 

      void hideCursor();
      void showCursor();
      void hideBorder();
      
      // data

      Window win;
      Display* disp;
      int screen;
      Pixmap pix;
      Imlib_Image image;
      ComThread* thread;
      int hideCursorDelay;
      int resize;
      int managed;
      int width;
      int height;
      int border;
      char dump[200];
      int showHelp;
      cMutex bufferLock;
      int vdrWidth;
      int vdrHeight;
      int cursorVisible;
      int borderVisible;
      int ignoreEsc;
      time_t lastMotion;

      static int eloquence;
};

//***************************************************************************
#endif  // __GRAPHTFT_HPP__
