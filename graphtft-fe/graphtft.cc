//***************************************************************************
// Group VDR/GraphTFT
// File graphtft.hpp
// Date 28.10.06 - Jörg Wendel
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
//--------------------------------------------------------------------------
// Class GrapTFT
//***************************************************************************

#include <X11/Xutil.h>
#include <X11/cursorfont.h>

#define XK_MISCELLANY
#include <X11/keysymdef.h>

#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h>

#include "graphtft.hpp"

//#define _DEBUG

//***************************************************************************
// Class GraphTft
//***************************************************************************

int GraphTft::eloquence = eloOff;

//***************************************************************************
// Object
//***************************************************************************

GraphTft::GraphTft()
{
   // init

   showHelp = false;
   resize = false;
   image = 0;
   hideCursorDelay = 0;
   managed = true;
   vdrWidth = 720;
   vdrHeight = 576;
   width = 720;
   height = 576;
   border = 0;
   *dump = 0;
   cursorVisible = yes;
   lastMotion = time(0);
   borderVisible = yes;
   ignoreEsc = no;
   screen = 0;

   thread = new ComThread();

   // the defaults

   thread->setHost("localhost");
   thread->setPort(2039);
}

GraphTft::~GraphTft()
{
   if (thread)
   {
      tell(eloAlways, "Stopping thread");

      thread->stop();

      delete thread;
   }
}

void GraphTft::setArgs(int argc, char* argv[])
{
   if (argc > 1 && (argv[1][0] == '?' || (strcmp(argv[1], "--help") == 0)))
   {
      showHelp = true;
      return ;
   }

   for (int i = 0; argv[i]; i++)
   {
      if (argv[i][0] != '-' || strlen(argv[i]) != 2)
         continue;

      switch (argv[i][1])
      {
         case 'i': if (argv[i+1]) ignoreEsc = yes;                         break;
         case 'h': if (argv[i+1]) thread->setHost(argv[i+1]);              break;
         case 'p': if (argv[i+1]) thread->setPort(atoi(argv[i+1]));        break;
         case 'e': if (argv[i+1]) setEloquence(atoi(argv[i+1]));           break;
         case 'W': if (argv[i+1]) width = atoi(argv[i+1]);                 break;
         case 'H': if (argv[i+1]) height = atoi(argv[i+1]);                break;
         case 'd': if (argv[i+1]) strcpy(dump, argv[i+1]);                 break;
         case 'c': if (argv[i+1]) hideCursorDelay = atoi(argv[i+1]);       break;
         case 'j': if (argv[i+1]) thread->setJpegQuality(atoi(argv[i+1])); break;

         case 'b': borderVisible = no; break;
         case 'n': managed = false;    break;
         case 'r': resize = true;      break;
      }
   }
}

//***************************************************************************
// Show Usage
//***************************************************************************

void GraphTft::showUsage()
{
   printf("Usage: graphtft-fe\n"
          "  Parameter:\n"
          "     -h <host>       vdr host no default, please specify\n"
          "     -p <port>       plugin port  (default 2039)\n"
          "     -e <eloquence>  log level    (default 0)\n"
          "     -W <width>      width        (default 720)\n"
          "     -H <height>     height       (default 576)\n"
          "     -d <file>       dump each image to file (default off)\n"
          "     -n              not managed  (default managed)\n"
          "     -r              resize image (default off)\n"
          "     -j <qunality>   JPEG quality (0-100)\n"
          "     -c <seconds>    hide mouse curser after <seconds>\n"
          "     -b              no boarder\n"
          "     -i              no exit on ESC key\n"
          "     ?, --help       this help\n"
      );
}

//***************************************************************************
// Start
//***************************************************************************

int GraphTft::start()
{
   if (showHelp)
   {
      showUsage();
      return 0;
   }

   if (init() != success)
      return fail;

   run();
   exit();

   return success;
}


#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

//***************************************************************************
// init/exit
//***************************************************************************

int GraphTft::init()
{
   Visual* vis;
   Colormap cm;
   int depth;

   // init X

   disp = XOpenDisplay(0);

   if (!disp)
   {
      printf("Invalid display, aborting\n");
      return fail;
   }

   // init communication thread

   thread->setClient(this);
   thread->Start();

   // init dispaly

   screen = DefaultScreen(disp);
   vis    = DefaultVisual(disp, screen);
   depth  = DefaultDepth(disp, screen);
   cm     = DefaultColormap(disp, screen);

   // create simple window

   if (managed)
   {
      const char* appName = "graphtft-fe";

      win = XCreateSimpleWindow(disp, DefaultRootWindow(disp),
                                0, 0, width, height, 0, 0, 0);

      XSetStandardProperties(disp, win, appName, appName, None,
                             0, 0, 0);


      XClassHint* classHint;
      XStoreName(disp, win, appName);

      /* set the name and class hints for the window manager to use */

      classHint = XAllocClassHint();

      if (classHint)
      {
          classHint->res_name = (char*)appName;
          classHint->res_class = (char*)appName;
      }

      XSetClassHint(disp, win, classHint);
      XFree(classHint);

      if (!borderVisible)
         hideBorder();
   }
   else
   {
      // create window more complex

      // attributes

      XSetWindowAttributes windowAttributes;

      windowAttributes.border_pixel      = BlackPixel(disp, screen);
      windowAttributes.border_pixmap	  = CopyFromParent;
      windowAttributes.background_pixel  = WhitePixel(disp, screen);
      windowAttributes.override_redirect = True;
      windowAttributes.bit_gravity       = NorthWestGravity;
      windowAttributes.event_mask        = ButtonPressMask | ButtonReleaseMask |
         KeyPressMask | ExposureMask | SubstructureNotifyMask;


      win = XCreateWindow(disp, RootWindow(disp, screen),
                          0, 0, width, height,
                          border, depth,
                          InputOutput,
                          vis,
                          CWBackPixel | CWBorderPixel | CWOverrideRedirect | CWBitGravity | CWEventMask,
                          &windowAttributes);


   }

   XSelectInput(disp, win,
                ButtonPressMask   |
                ButtonReleaseMask |
                PointerMotionMask |
                KeyPressMask      |
                ClientMessage     |
                SubstructureNotifyMask |
                ExposureMask);          // events to receive


   XMapWindow(disp, win);               // show
   XFlush(disp);

   Screen* scn = DefaultScreenOfDisplay(disp);
   pix = XCreatePixmap(disp, win, width, height, DefaultDepthOfScreen(scn));

   imlib_set_cache_size(16 * 1024 * 1024);
   imlib_set_color_usage(256);

   imlib_context_set_dither(0);         // dither for depths < 24bpp
   imlib_context_set_display(disp);     // set the display
   imlib_context_set_visual(vis);       // visual,
   imlib_context_set_colormap(cm);      // colormap

   // imlib_context_set_drawable(win);  // and the drawable we are using
   imlib_context_set_drawable(pix);     // and the drawable we are using

   return 0;
}

void GraphTft::hideBorder()
{
   struct MwmHints
   {
      int flags;
      int functions;
      int decorations;
      int input_mode;
      int status;
   };

   MwmHints mwmhints;
   Atom prop;

   memset(&mwmhints, 0, sizeof(mwmhints));
   mwmhints.flags = 1L << 1;
   mwmhints.decorations = 0;

   prop = XInternAtom(disp, "_MOTIF_WM_HINTS", False);

   XChangeProperty(disp, win, prop, prop, 32, PropModeReplace,
                   (unsigned char*)&mwmhints, sizeof(mwmhints)/sizeof(long));
}

int GraphTft::exit()
{
   XFreePixmap(disp, pix);
   XCloseDisplay(disp);
   imlib_free_image();

   return 0;
}

//***************************************************************************
// Send Exent
//***************************************************************************

int GraphTft::sendEvent()
{
   XEvent ev;
   Display* d;

   if ((d = XOpenDisplay(0)) == 0)
   {
      tell(eloAlways, "Error: Sending event failed, cannot open display");
      return fail;
   }

   ev.type = Expose;
   XSendEvent(d, win, False, 0, &ev);

   XCloseDisplay(d);

   return success;
}

int fromJpeg(Imlib_Image& image, unsigned char* buffer, int size)
{
   struct jpeg_decompress_struct cinfo;
   struct jpeg_error_mgr jerr;
   int w, h;
   DATA8 *ptr, *line[16], *data;
   DATA32 *ptr2, *dest;
   int x, y;

   cinfo.err = jpeg_std_error(&jerr);

   jpeg_create_decompress(&cinfo);
   jpeg_mem_src(&cinfo, buffer, size);
   jpeg_read_header(&cinfo, TRUE);
   cinfo.do_fancy_upsampling = FALSE;
   cinfo.do_block_smoothing = FALSE;

   jpeg_start_decompress(&cinfo);

   w = cinfo.output_width;
   h = cinfo.output_height;

   image = imlib_create_image(w, h);
   imlib_context_set_image(image);

   dest = ptr2 = imlib_image_get_data();
   data = (DATA8*)malloc(w * 16 * cinfo.output_components);

   for (int i = 0; i < cinfo.rec_outbuf_height; i++)
      line[i] = data + (i * w * cinfo.output_components);

   for (int l = 0; l < h; l += cinfo.rec_outbuf_height)
   {
      jpeg_read_scanlines(&cinfo, line, cinfo.rec_outbuf_height);
      int scans = cinfo.rec_outbuf_height;

      if (h - l < scans)
         scans = h - l;

      ptr = data;

      for (y = 0; y < scans; y++)
      {
         for (x = 0; x < w; x++)
         {
            *ptr2 = (0xff000000) | ((ptr[0]) << 16) | ((ptr[1]) << 8) | (ptr[2]);
            ptr += cinfo.output_components;
            ptr2++;
         }
      }
   }

   free(data);

   imlib_image_put_back_data(dest);

   jpeg_finish_decompress(&cinfo);
   jpeg_destroy_decompress(&cinfo);

   return success;
}

//***************************************************************************
// Load Image
//***************************************************************************

void GraphTft::updateImage(const unsigned char* buffer, int size)
{
   tell(eloAlways, "update image");

   bufferLock.Lock();

   if (image)
   {
      imlib_context_set_image(image);
      imlib_free_image();
   }

#ifdef _DEBUG

   tell(eloAlways, "loading image, from file");

   image = imlib_load_image("test.jpg");
   sendEvent();
   dumpImage(image);

   bufferLock.Unlock();

   return ;
#endif

   tell(eloAlways, "loading image, size (%d)", size);

   if (size)
   {
      fromJpeg(image, (unsigned char*)buffer, size);

      dumpImage(image);
      sendEvent();
   }

   bufferLock.Unlock();
}

//***************************************************************************
// Paint
//***************************************************************************

int GraphTft::paint()
{
   XWindowAttributes windowAttributes;

   if (!image)
      return fail;

   tell(eloAlways, "paint ...");

   // get actual window size

   XGetWindowAttributes(disp, win, &windowAttributes);
   width = windowAttributes.width;
   height = windowAttributes.height;

   // lock buffer

   bufferLock.Lock();

   imlib_context_set_image(image);

   // get VDR's image size

   vdrWidth = imlib_image_get_width();
   vdrHeight = imlib_image_get_height();

   if (!resize)
   {
      imlib_render_image_on_drawable(0, 0);    // render image on drawable
   }
   else
   {
      Imlib_Image buffer;

      buffer = imlib_create_image(width, height);

      imlib_context_set_image(buffer);

      imlib_blend_image_onto_image(image, 0,
                                   0, 0, vdrWidth, vdrHeight,
                                   0, 0, width, height);

      imlib_render_image_on_drawable(0, 0);
      imlib_free_image();
   }

   XSetWindowBackgroundPixmap(disp, win, pix);
   XClearWindow(disp, win);

   bufferLock.Unlock();

   return success;
}

//***************************************************************************
// Dump Image
//***************************************************************************

void GraphTft::dumpImage(Imlib_Image image)
{
   if (*dump)
   {
      imlib_context_set_image(image);
      imlib_save_image(dump);
   }
}

//***************************************************************************
// Run loop
//***************************************************************************

int GraphTft::run()
{
   XEvent ev;
   KeySym key_symbol;
   int running = true;
   int update = false;

   while (running)
   {
      while (XPending(disp))
      {
         XNextEvent(disp, &ev);

         switch (ev.type)
         {
            case Expose:        update = true;               break;
            case CreateNotify:  tell(eloAlways, "Create");   break;
            case DestroyNotify: tell(eloAlways, "Destroy");  break;
            case MotionNotify:
               onMotion();
               onButtonPress(ev, na);       break;
            case ButtonRelease: onButtonPress(ev, no);       break;
            case ButtonPress:   onButtonPress(ev, yes);      break;

            case KeyPress:
            {
               key_symbol = XKeycodeToKeysym(disp, ev.xkey.keycode, 0);
               tell(eloAlways, "Key (%ld) pressed", key_symbol);

               if (key_symbol == XK_Escape && !ignoreEsc)
                  running = false;
               else
                  onKeyPress(ev);
               break;
            }

            default:
               break;
         }
      }

      if (update)
      {
         update = false;
         paint();
      }

      // check mouse cursor

      if (hideCursorDelay && lastMotion < time(0) - hideCursorDelay && cursorVisible)
         hideCursor();

      if (!XPending(disp))
         usleep(10000);
   }

   return 0;
}

//***************************************************************************
// On Motion
//***************************************************************************

int GraphTft::onMotion()
{
   lastMotion = time(0);

   if (!cursorVisible)
      showCursor();

   return done;
}

//***************************************************************************
// On key Press (keyboard)
//***************************************************************************

int GraphTft::onKeyPress(XEvent event)
{
   int x = event.xmotion.x;
   int y = event.xmotion.y;
   int flag = ComThread::efKeyboard;
   int button = event.xkey.keycode;

   thread->mouseEvent(x, y, button, flag);

   return 0;
}

//***************************************************************************
// On Button Press (mouse)
//***************************************************************************

int GraphTft::onButtonPress(XEvent event, int press)
{
   static long lastTime = 0;
   static int lastButton = na;
   static int lastPressX = 0;
   static int lastPressY = 0;
   static int lastPressed = na;

   int x = event.xmotion.x;
   int y = event.xmotion.y;
   int flag = 0;

   if (press != na)
      tell(eloAlways, "Button '%s' at (%d/%d) button %d, time (%ld)",
           press ? "press" : "release",
           event.xmotion.x, event.xmotion.y,
           event.xbutton.button,
           event.xbutton.time);

   if (resize)
   {
      x = (int)(((double)event.xmotion.x / (double)width) * (double)vdrWidth);
      y = (int)(((double)event.xmotion.y / (double)height) * (double)vdrHeight);
   }

   if (press == no)
   {
      // on button release

      if (abs(y - lastPressY) < 5 && abs(x - lastPressX) < 5)
      {
         if (lastButton == (int)event.xbutton.button
             && event.xbutton.button == cGraphTftComService::mbLeft
             && event.xbutton.time-lastTime < 300)
         {
            tell(eloAlways, "assuming double-click");
            flag |= cGraphTftComService::efDoubleClick;
         }

         thread->mouseEvent(x, y, event.xbutton.button, flag);

         lastTime = event.xbutton.time;
         lastButton = event.xbutton.button;
      }

      lastPressed = press;
   }
   else if (press == na && lastPressed == yes)
   {
      // no Button action, only motion with pressed button

      if (abs(y - lastPressY) > 5 || abs(x - lastPressX) > 5)
      {
         if (abs(y - lastPressY) > abs(x - lastPressX))
         {
            tell(eloAlways, "V-Whipe of (%d) pixel detected", y - lastPressY);
            thread->mouseEvent(x, y,
                               lastButton, cGraphTftComService::efVWhipe,
                               y - lastPressY);
         }
         else
         {
            tell(eloAlways, "H-Whipe of (%d) pixel detected", x - lastPressX);
            thread->mouseEvent(x, y,
                               lastButton, cGraphTftComService::efHWhipe,
                               x - lastPressX);
         }

         lastPressX = x;
         lastPressY = y;
      }
   }
   else if (press == yes)
   {
      // on button press

      lastPressX = x;
      lastPressY = y;
      lastPressed = press;
   }

   return success;
}

//***************************************************************************
// Hide Cursor
//***************************************************************************

void GraphTft::hideCursor()
{
   // Hide the cursor

   Cursor invisibleCursor;
   Pixmap bitmapNoData;
   XColor black;

   static char noData[] = { 0,0,0,0,0,0,0,0 };
   black.red = black.green = black.blue = 0;

   tell(eloAlways, "Hide mouse cursor");

   bitmapNoData = XCreateBitmapFromData(disp, win, noData, 8, 8);
   invisibleCursor = XCreatePixmapCursor(disp, bitmapNoData, bitmapNoData,
                                         &black, &black, 0, 0);
   XDefineCursor(disp, win, invisibleCursor);
   XFreeCursor(disp, invisibleCursor);

   cursorVisible = no;
}

//***************************************************************************
// Show Cursor
//***************************************************************************

void GraphTft::showCursor()
{
   // Restore the X left facing cursor

   Cursor cursor;

   tell(eloAlways, "Show mouse cursor");

   cursor = XCreateFontCursor(disp, XC_left_ptr);
   XDefineCursor(disp, win, cursor);
   XFreeCursor(disp, cursor);

   cursorVisible = yes;
}
