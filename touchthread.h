/*
 * touchthread.h: A plugin for the Video Disk Recorder
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

#ifndef _TOUCHTHREAD_H_
#define _TOUCHTHREAD_H_

#include <vdr/thread.h>

#include <common.h>

//***************************************************************************
// Class Touch Thread
//***************************************************************************

class cTouchThread : public cThread
{
   public:
  
      // definitions

      enum Misc
      {
         sizeBuffer = 32,
         bounceTime = 30,      // milli Seconds (0.03 sec)
         doubleClickTime = 500 // milli Seconds (0.5 sec)
      };

      struct CalibrationSetting
      {
         int swapXY;
         double scaleX;
         double scaleY;
         int offsetX;
         int offsetY;
         int scaleWidth;
         int scaleHeight;
      };

      // object

      cTouchThread(void* aDisplay);
      virtual ~cTouchThread();

      // frame

      void Stop();
      void Action(void);

      // interface

      void setCalibrate(int state)              { calibrate = state; }
      int getCalibrate()                        { return calibrate; }
      void setSetting(CalibrationSetting* aSet) { settings = *aSet; }
      void resetSetting(int swap = no);
      CalibrationSetting* getSettings()         { return &settings; }
      void setDevice(const char* aDevice, int doOpen = no);

      int open();
      int close();
      int isOpen()    { return handle > 0; }

   protected:

      void processEvent(bool touch, int x, int y);
      void rescale();

      // data

      CalibrationSetting settings;
      int handle;
      void* display;
      int calibrate;
      char* touchDevice;
};

//***************************************************************************
#endif // _TOUCHTHREAD_H_
