/*
 * touchthread.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *  (c) 2007-2008 Jörg Wendel
 *
 * Date: 14.11.2008
 *
 * The touch device driver is taken from the 
 *     touchTFT plugin written by (c) Frank Simon
 *
 */

//***************************************************************************
// Includes
//***************************************************************************

#include <linux/input.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>

#include <vdr/remote.h>
#include <vdr/tools.h>

#include "touchthread.h"
#include "setup.h"
#include "display.h"

//***************************************************************************
// Class Touch Thread
//***************************************************************************

cTouchThread::cTouchThread(void* aDisplay)
   : cThread("TouchTFT-Thread")
{
   display = aDisplay;
   handle = na;
   calibrate = no;
   touchDevice = 0;
};

cTouchThread::~cTouchThread()
{
   Stop();
   close();

   if (touchDevice) free(touchDevice);
}

//***************************************************************************
// Reset Settings
//***************************************************************************

void cTouchThread::resetSetting(int swap)
{
   settings.offsetX = 0;
   settings.offsetY = 0;
   settings.scaleX = 1.0;
   settings.scaleY = 1.0;
   settings.swapXY = swap;         
   settings.scaleWidth = Thms::theTheme->getWidth();
   settings.scaleHeight = Thms::theTheme->getHeight();
}

//***************************************************************************
// Stop the Thread
//***************************************************************************

void cTouchThread::Stop()
{
   Cancel(3);
}

//***************************************************************************
// Device
//***************************************************************************

void cTouchThread::setDevice(const char* aDevice, int doOpen)
{
   if (Str::isEmpty(aDevice))
      return ;

   if (!touchDevice || strcmp(touchDevice, GraphTFTSetup.touchDevice) != 0)
   {
      if (touchDevice) free(touchDevice);

      touchDevice = strdup(GraphTFTSetup.touchDevice);

      if (doOpen)
      {
         Cancel(3);
         close();

         if (open() == success)
            Start();
      }
   }
}

//***************************************************************************
// Open / Close
//***************************************************************************

int cTouchThread::open()
{
   if (Str::isEmpty(touchDevice))
   {
      tell(0, "Can't open touch device, now device configured");
      return fail;
   }

   // open the device ..

   handle = ::open(touchDevice, O_RDWR);

   if (handle < 0)
   {
      tell(0, "Error: Opening device '%s' failed, errno was (%d) '%s'", 
           touchDevice, errno, strerror(errno));

      handle = na;

      return fail;
   }

   tell(0, "Open touch device '%s'", touchDevice);

   return success;
}

int cTouchThread::close()
{
   // wait for Thread exit

   if (isOpen())
      ::close(handle);

   handle = na;
   tell(0, "Device '%s' closed", touchDevice);

   return done;
}

//***************************************************************************
// Action
//***************************************************************************

void cTouchThread::Action(void)
{
   struct input_event touchdata[sizeBuffer];

   int x = 0;
   int y = 0;
   int r = 0;
   int num = 0;
   bool touched = no;

   open();

   while (Running())
   {
      if (!cFile::FileReady(handle, 100))
         continue ;

      // read

      r = safe_read(handle, touchdata, sizeof(input_event) * sizeBuffer);

      if (r < 0)
      {
         tell(0, "Fatal: Touch device read failed, errno (%d) '%s'", 
              errno, strerror(errno));

         sleep(10);
         continue ;
      }

      num = r / sizeof(input_event);

      for (int i = 0; i < num; i++)
      {
         switch (touchdata[i].type)
         {
            case EV_ABS:
            {
               // received a coordinate

               if ((touchdata[i].code == 0 && !settings.swapXY)
                   || (touchdata[i].code != 0 && settings.swapXY))
                  y = touchdata[i].value;
               else
                  x = touchdata[i].value;

               tell(3, "ABS: (%d/%d)", x, y);

               break;
            }
            case EV_KEY:
            {
               if (touchdata[i].code == BTN_TOUCH     // BTN_TOUCHED
                   || touchdata[i].code == BTN_LEFT)  // BTN_LEFT
               {
                  tell(3, "KEY: value (%d) code (%d)", 
                       touchdata[i].value, touchdata[i].code); 

                  touched = touchdata[i].value != 0;
               }
                     
               break;
            }
                     
            case EV_SYN:
            {
               // data complete

               if (touched)
                  tell(3, "SYN: (%d/%d) touched (%d)", x, y, touched);
               else
                  tell(0, "SYN: (%d/%d) released (%d)", x, y, touched);

               processEvent(touched, x, y);

               break;
            }
         }
      }
   }

   close();

   isyslog("GraphTFT plugin touch-thread thread ended (pid=%d)", getpid());
   tell(0, "Touch thread ended");
}

//***************************************************************************
// Rescale to actual theme size
//***************************************************************************

void cTouchThread::rescale()
{
   if (settings.scaleWidth == Thms::theTheme->getWidth())
      return ;

   settings.scaleX = (settings.scaleX * (double)Thms::theTheme->getWidth()) 
      / settings.scaleWidth;
   settings.scaleY = (settings.scaleY * (double)Thms::theTheme->getHeight()) 
      / settings.scaleHeight;

   settings.scaleWidth = Thms::theTheme->getWidth();
   settings.scaleHeight = Thms::theTheme->getHeight();

   tell(0, "Rescaled touch calibration to scale values (%f/%f)",
        settings.scaleX, settings.scaleY);

   ((cGraphTFTDisplay*)display)->calibration.settings = settings;
   GraphTFTSetup.touchSettings = settings;
   GraphTFTSetup.Store();
}

//***************************************************************************
// Process Event
//***************************************************************************

void cTouchThread::processEvent(bool touch, int x, int y)
{
   static int lastX = 0;
   static int lastY = 0;  
   static uint64_t lastTouchRelease = 0;

   int flag = 0;

   // check if rescale is required ...

   if (!calibrate && settings.scaleWidth != Thms::theTheme->getWidth())
      rescale();

   // ignore 'press' events, we triggerd only on 'release' events
   // -> touch is 0 on release

   if (!touch)
   {
      // bounce check
      
      if (lastTouchRelease + bounceTime <= cTimeMs::Now())
      {
         // scale only if not in calibration mode !

         if (!calibrate)
         {
            int _x = x; 
            int _y = y;

            x = (int)(((double)(_x + settings.offsetX)) * settings.scaleX);
            y = (int)(((double)(_y + settings.offsetY)) * settings.scaleY);

            tell(2, "Touch scaled from (%d/%d) to (%d/%d)", 
                 _x, _y, x, y);
         }

         if (cTimeMs::Now() - lastTouchRelease < doubleClickTime
             && abs(lastX-x) < 20 
             && abs(lastY-y) < 20)
         {
            tell(0, "Assuming double-click");
            flag |= cGraphTftComService::efDoubleClick;
         }

         ((cGraphTFTDisplay*)display)->mouseEvent(x, y, 
                                cGraphTftComService::mbLeft, 
                                flag);

         // notice last values

         lastX = x; 
         lastY = y;
         lastTouchRelease = cTimeMs::Now();
      }
   }
}
