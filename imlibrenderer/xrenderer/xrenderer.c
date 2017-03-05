//***************************************************************************
// Group VDR/GraphTFT
// File xrenderer.c
// Date 16.02.14 - Jörg Wendel
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
//--------------------------------------------------------------------------
// Class XRenderer
//***************************************************************************

#include <stdio.h>

#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#define XK_MISCELLANY
#include <X11/keysymdef.h>

#include <common.h>
#include "xrenderer.h"

//***************************************************************************
// Object
//***************************************************************************

XRenderer::XRenderer(int x, int y, int width, int height, 
                     string cfgPath, int utf, string thmPath)
   : ImlibRenderer(x, y, width, height, cfgPath, utf, thmPath)
{
   disp = 0;
   win = 0;
   pix = 0;
   screen = 0;
   initialized = no;
}

XRenderer::~XRenderer()
{
	deinit();
}

void XRenderer::setDisplaySize(int width, int height)
{
   if (dspWidth != width || dspHeight != height)
   {
      cMutexLock lock(&mutex);

      tell(0, "Changing display size to %dx%d", width, height);      
      deinit();
      dspWidth = width;
      dspHeight = height;
      init(no);
   }
}

//***************************************************************************
// Init
//***************************************************************************

int XRenderer::init(int lazy)
{
   const char* appName = "graphtft-fe";

   Visual* vis;
   Colormap cm;
   int depth;
   XClassHint* classHint;

   cMutexLock lock(&mutex);

   ImlibRenderer::init(lazy);

   if (Str::isEmpty(devname))
   {
      tell(0, "Can't open display 'null' continue in detached mode!");
      return fail;
   }

   // init X 

   if (!(disp = XOpenDisplay(devname)))
   {
      tell(0, "Can't open display '%s' continue in detached mode", devname);
      return lazy ? success : fail;
   }

   if (!XInitThreads()) 
      tell(0, "Can't initialize X11 thread support");

   // init display

   screen = DefaultScreen(disp);
   vis    = DefaultVisual(disp, screen);
   depth  = DefaultDepth(disp, screen);
   cm     = DefaultColormap(disp, screen);

   // create simple window

   win = XCreateSimpleWindow(disp, DefaultRootWindow(disp), 
                             0, depth, dspWidth, dspHeight, 0, 0, 0);
  
   XSetStandardProperties(disp, win, appName, appName, None, 0, 0, 0);
   XStoreName(disp, win, appName);
  
   /* set the name and class hints for the window manager to use */
   
   classHint = XAllocClassHint();
   classHint->res_name = (char*)appName;
   classHint->res_class = (char*)appName;
   XSetClassHint(disp, win, classHint);
   XFree(classHint);
   
   XSelectInput(disp, win, 
                ButtonPressMask   |
                ButtonReleaseMask | 
                PointerMotionMask | 
                KeyPressMask      |
                ClientMessage     |
                SubstructureNotifyMask |
                ExposureMask);                    // events to receive

   XMapWindow(disp, win);                         // show
   XFlush(disp);

   Screen* scn = DefaultScreenOfDisplay(disp);
   pix = XCreatePixmap(disp, win, dspWidth, dspHeight, DefaultDepthOfScreen(scn));

   imlib_context_set_dither(1);         // dither for depths < 24bpp 
   imlib_context_set_display(disp);     // set the display
   imlib_context_set_visual(vis);       // visual,
   imlib_context_set_colormap(cm);      // colormap
   imlib_context_set_drawable(pix);     // and drawable we are using 

//    {
//       XWindowAttributes windowAttributes;
//       XGetWindowAttributes(disp, win, &windowAttributes);
//       int w = windowAttributes.width; 
//       int h = windowAttributes.height;
      
//       tell(0, "Created window with (%d/%d) got (%d/%d)", dspWidth, dspHeight, w, h);
//    }

   tell(0, "Connection to '%s' established", devname);
   initialized = yes;

   return success;
}

//***************************************************************************
// Deinit
//***************************************************************************

void XRenderer::deinit()
{
   cMutexLock lock(&mutex);

   ImlibRenderer::deinit();

   if (initialized)
   {
      imlib_context_disconnect_display();

      if (win)  XDestroyWindow(disp, win);
      if (pix)  XFreePixmap(disp, pix);

      XFlush(disp);

      if (XCloseDisplay(disp))
         tell(0, "Error closing display");

      disp = 0;
      win = 0;
      pix = 0;
      screen = 0;
      initialized = no;
   }

   tell(0, "Connection to '%s' closed, now detached", devname);
}

//***************************************************************************
// attach / detach
//***************************************************************************

int XRenderer::attach(const char* disp)   
{ 
   if (initialized)
   {
      tell(0, "Already attached, trying detach first");
      detach();
   }

   if (!Str::isEmpty(disp))
      setDevName(disp);

   tell(0, "Try to attach to '%s'", devname);

   return init(no);
}

int XRenderer::detach()
{ 
   if (!initialized)
   {
      tell(0, "Already detached");
      return done;
   }

   tell(0, "Detach from '%s'", devname);
   deinit();

   return success;
}

//***************************************************************************
// X Pending
//***************************************************************************

int XRenderer::xPending()
{
   cMutexLock lock(&mutex);
   XEvent ev;

   if (!initialized)
      return success;

   while (XPending(disp))
      XNextEvent(disp, &ev);

   return success;
}

//***************************************************************************
// Refresh
//***************************************************************************

void XRenderer::refresh(int force)
{
   cMutexLock lock(&mutex);

   if (!force || !initialized)    // don't need 'total' refresh every time since we refresh the areas
      return;

   tell(2, "refresh all");

   refreshPixmap();

   XClearWindow(disp, win);
}

void XRenderer::refreshArea(int x, int y, int width, int height)
{
   cMutexLock lock(&mutex);

   if (!initialized)
      return ;

   refreshPixmap();

   // scale coordinates from theme to display size

   double xScale = (double)dspWidth / (double)themeWidth;
   double yScale = (double)dspHeight / (double)themeHeight;

   int xDsp = x * xScale;
   int yDsp = y * yScale;
   int widthDsp = width * xScale;
   int heightDsp = height * yScale;

   tell(3, "Refresh area at %d/%d (%d/%d); scaled to %d/%d (%d/%d); scale %.2f/%.2f; dspSize (%d/%d)",
        x, y, width, height, xDsp, yDsp, widthDsp, heightDsp,
        xScale, yScale, dspWidth, dspHeight);
   
   XClearArea(disp, win, xDsp, yDsp, widthDsp, heightDsp, false);
}

void XRenderer::refreshPixmap()
{
   //  XWindowAttributes windowAttributes;
   //  XGetWindowAttributes(disp, win, &windowAttributes);
   //  int width = windowAttributes.width; 
   //  int height = windowAttributes.height;

   imlib_context_set_image(_cur_image);
   imlib_render_image_on_drawable_at_size(0, 0, dspWidth, dspHeight);

   XSetWindowBackgroundPixmap(disp, win, pix);
}

void XRenderer::clear()
{
   if (!initialized)
      return ;

   cMutexLock lock(&mutex);
	ImlibRenderer::clear();
}
