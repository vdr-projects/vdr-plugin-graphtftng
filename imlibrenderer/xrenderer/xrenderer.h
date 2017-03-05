//***************************************************************************
// Group VDR/GraphTFT
// File xrenderer.h
// Date 09.11.14 - Jörg Wendel
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
//--------------------------------------------------------------------------
// Class XRenderer
//***************************************************************************

#ifndef __DXRENDERER_H__
#define __DXRENDERER_H__

#include <X11/Xlib.h>

#include <vdr/thread.h>
#include "imlibrenderer.h"

//***************************************************************************
// Class X Renderer
//***************************************************************************

class XRenderer : public ImlibRenderer
{
   public:	

      XRenderer(int x, int y, int width, int height, string cfgPath, int utf, string thmPath);
      ~XRenderer();

      void setDisplaySize(int width, int height);
		int init(int lazy);
		void deinit();
      int xPending();
		void refresh(int force = no);
      void refreshArea(int x, int y, int width, int height);
		void clear();

      virtual int attach(const char* disp = 0);
      virtual int detach();
      
   protected:

      void refreshPixmap();

      Window win;
      Display* disp;
      int screen;
      Pixmap pix;
      cMutex mutex;
      int initialized;
};

//***************************************************************************
#endif // __DXRENDERER_H__
