/**
 *  GraphTFT plugin for the Video Disk Recorder 
 * 
 *  setup.h - A plugin for the Video Disk Recorder
 *
 *  (c) 2004-2013 Lars Tegeler, Sascha Volkenandt, Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 **/

#ifndef __GTFT_SETUP_H
#define __GTFT_SETUP_H

#include "display.h"

//***************************************************************************
// Class Graph TFT Setup 
//***************************************************************************

class cGraphTFTSetup 
{
   public:

      // definitions

      enum Size
      {
         sizePath = 255
      };

      cGraphTFTSetup();
      ~cGraphTFTSetup() {}

      string Theme;           // name of the actual theme
      int HideMainMenu;
      int xBorder;
      int yBorder;
      int xOffset;
      int yOffset;
      int DumpImage;
      int DumpRefresh;
      int Level;
      int LogDevice;
      int Iso2Utf;

      int enableSpectrumAnalyzer;
      int index;
      int JpegQuality;
      int flipOSD;
      int width;
      int height;
      // int redrawEvery;
      string normalMode;
      bool storeNormalMode;
      string originalNormalMode;
      string themesPath;
      string configPath;
      int snapshotWidth;
      int snapshotHeight;
      int snapshotQuality;
      char snapshotPath[sizePath+TB];
      char touchDevice[sizePath+TB];
      cTouchThread::CalibrationSetting touchSettings;

      bool SetupParse(const char* Name, const char* Value);
      void Store(int force = no);
      void setClient(cPlugin* aClient) { plugin = aClient; }

   protected:

      cPlugin* plugin;
};

//***************************************************************************
// Class Menu Setup GraphTFT
//***************************************************************************

class cMenuSetupGraphTFT : public cMenuSetupPage
{
   public:

      cMenuSetupGraphTFT(cGraphTFTDisplay* aDisplay);
      virtual ~cMenuSetupGraphTFT();

      virtual eOSState ProcessKey(eKeys Key);
      void Store();
      void setHelp();

   protected:

      cGraphTFTDisplay* display;

      // data

      char* themeNames[cGraphTFTSetup::sizePath+TB];
};

//***************************************************************************

extern cGraphTFTSetup GraphTFTSetup;

//***************************************************************************
#endif // __GTFT_SETUP_H
