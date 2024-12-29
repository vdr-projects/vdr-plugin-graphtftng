/**
 *  GraphTFT plugin for the Video Disk Recorder 
 * 
 *  setup.c - A plugin for the Video Disk Recorder
 *
 *  (c) 2006-2013 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 **/

#include "common.h"
#include "setup.h"

//***************************************************************************
// Log Devices
//***************************************************************************

static const char* logDevices[] = 
{
   "none",
   "stdout",
   "syslog",
   "file"
};

cGraphTFTSetup GraphTFTSetup;

//***************************************************************************
// Object
//***************************************************************************

cGraphTFTSetup::cGraphTFTSetup()
{
    Theme = "";
    index = 0;
    Level=0;
    LogDevice=0;
    Iso2Utf = yes;
    HideMainMenu = 0;
    yBorder = 5;
    xBorder = 25;
    xOffset = 0;
    yOffset = 0;
    DumpImage = 0;
    DumpRefresh = 5;
    JpegQuality = 60;
    enableSpectrumAnalyzer = 0;
    normalMode = "Standard";
    storeNormalMode = true;
    originalNormalMode = "";
    width = 800;
    height = 600;
    themesPath = "";
    configPath = "";
    snapshotWidth = 240;
    snapshotHeight = 180;
    snapshotQuality = 90;
    strcpy(snapshotPath, "/tmp");
    strcpy(touchDevice, "/dev/input/event3");   
}

//***************************************************************************
// Setup Parse
//***************************************************************************

bool cGraphTFTSetup::SetupParse(const char* Name, const char* Value)
{
   if      (!strcasecmp(Name, "Theme"))             Theme = Value;
   else if (!strcasecmp(Name, "HideMainMenu"))      HideMainMenu  = atoi(Value);
   else if (!strcasecmp(Name, "Iso2Utf"))           Iso2Utf  = atoi(Value);
   else if (!strcasecmp(Name, "SpectrumAnalyzer"))  enableSpectrumAnalyzer  = atoi(Value);
   else if (!strcasecmp(Name, "XBorder"))           xBorder  = atoi(Value);
   else if (!strcasecmp(Name, "YBorder"))           yBorder  = atoi(Value);
   else if (!strcasecmp(Name, "XOffset"))           xOffset  = atoi(Value);
   else if (!strcasecmp(Name, "YOffset"))           yOffset  = atoi(Value);
   else if (!strcasecmp(Name, "DumpImage"))         DumpImage  = atoi(Value);
   else if (!strcasecmp(Name, "DumpRefresh"))       DumpRefresh  = atoi(Value);
   else if (!strcasecmp(Name, "LogDevice"))         LogDevice  = atoi(Value);
   else if (!strcasecmp(Name, "Level"))             Level  = atoi(Value);
   else if (!strcasecmp(Name, "JpegQuality"))       JpegQuality  = atoi(Value);
   else if (!strcasecmp(Name, "flipOSD"))           flipOSD  = atoi(Value);
   else if (!strcasecmp(Name, "normalMode"))        normalMode = Value;
   else if (!strcasecmp(Name, "width"))             width = atoi(Value);
   else if (!strcasecmp(Name, "height"))            height = atoi(Value);

   else if (!strcasecmp(Name, "snapshotWidth"))     snapshotWidth = atoi(Value);
   else if (!strcasecmp(Name, "snapshotHeight"))    snapshotHeight = atoi(Value);
   else if (!strcasecmp(Name, "snapshotQuality"))   snapshotQuality = atoi(Value);
   else if (!strcasecmp(Name, "snapshotPath"))      strcpy(snapshotPath, Value);

   else if (!strcasecmp(Name, "touchDevice"))       strcpy(touchDevice, Value);
   else if (!strcasecmp(Name, "touchYOffset"))      touchSettings.offsetY = atoi(Value);
   else if (!strcasecmp(Name, "touchXOffset"))      touchSettings.offsetX = atoi(Value);
   else if (!strcasecmp(Name, "touchSwapXY"))       touchSettings.swapXY = atoi(Value);
   else if (!strcasecmp(Name, "touchXScale"))       touchSettings.scaleX = atof(Value);
   else if (!strcasecmp(Name, "touchYScale"))       touchSettings.scaleY = atof(Value);
   else if (!strcasecmp(Name, "touchScaleWidth"))   touchSettings.scaleWidth = atoi(Value);
   else if (!strcasecmp(Name, "touchScaleHeight"))  touchSettings.scaleHeight = atoi(Value);

   else return false;

   return true;
}

//***************************************************************************
// Store
//***************************************************************************

void cGraphTFTSetup::Store(int force)
{
   if (Thms::theTheme != themes.Get(index))
   {
      Thms::theTheme = themes.Get(index);
      Thms::theTheme->checkViewMode();
   }

   Theme = Thms::theTheme->getName();

   plugin->SetupStore("Theme", Theme.c_str());
   plugin->SetupStore("HideMainMenu", HideMainMenu);
   plugin->SetupStore("Iso2Utf", Iso2Utf);
   plugin->SetupStore("SpectrumAnalyzer", enableSpectrumAnalyzer);
   plugin->SetupStore("XBorder", xBorder);
   plugin->SetupStore("YBorder", yBorder);
   plugin->SetupStore("XOffset", xOffset);
   plugin->SetupStore("YOffset", yOffset);
   plugin->SetupStore("DumpImage", DumpImage);
   plugin->SetupStore("DumpRefresh", DumpRefresh);
   plugin->SetupStore("LogDevice", LogDevice);
   plugin->SetupStore("Level", Level);
   plugin->SetupStore("JpegQuality", JpegQuality);
   plugin->SetupStore("flipOSD", flipOSD);
   plugin->SetupStore("Width", width);
   plugin->SetupStore("Height", height);

   if (storeNormalMode)
      plugin->SetupStore("normalMode", normalMode.c_str());
   else if (!originalNormalMode.empty())
      plugin->SetupStore("normalMode", originalNormalMode.c_str());

   plugin->SetupStore("snapshotWidth",    snapshotWidth);
   plugin->SetupStore("snapshotHeight",   snapshotHeight);
   plugin->SetupStore("snapshotQuality",  snapshotQuality);
   plugin->SetupStore("snapshotPath",     snapshotPath);

   plugin->SetupStore("touchDevice",      touchDevice);
   plugin->SetupStore("touchSwapXY",      touchSettings.swapXY);
   plugin->SetupStore("touchXOffset",     touchSettings.offsetX);
   plugin->SetupStore("touchYOffset",     touchSettings.offsetY);
   plugin->SetupStore("touchScaleWidth",  touchSettings.scaleWidth);
   plugin->SetupStore("touchScaleHeight", touchSettings.scaleHeight);

   char* tmp;

   asprintf(&tmp, "%f", touchSettings.scaleX);
   plugin->SetupStore("touchXScale",  tmp);
   free(tmp);

   asprintf(&tmp, "%f", touchSettings.scaleY);
   plugin->SetupStore("touchYScale",  tmp);
   free(tmp);

   if (force)
      Setup.Save();
}

//***************************************************************************
// cMenuSetupGraphTFT
//***************************************************************************

cMenuSetupGraphTFT::cMenuSetupGraphTFT(cGraphTFTDisplay* aDisplay)
{
   cGraphTFTTheme* t;
   char* buf = 0;
   int i;

   for (int i = 0; i < 100; i++)
      themeNames[i] = 0;

   display = aDisplay;

   SetSection(tr("GraphTFT"));

   GraphTFTSetup.index = Thms::theTheme->Index();
   tell(0, "Theme index is (%d)", GraphTFTSetup.index);

   for (i = 0, t = themes.First(); t && i < 100; t = themes.Next(t), i++)
      asprintf(&themeNames[i], "%s", t->getName().c_str());

   if (themes.Count())
      Add(new cMenuEditStraItem(tr("Theme"), &GraphTFTSetup.index, themes.Count(), themeNames));

   Add(new cMenuEditBoolItem(tr("Hide Mainmenu Entry"),        &GraphTFTSetup.HideMainMenu));
   Add(new cMenuEditBoolItem(tr("VDR use iso charset"),         &GraphTFTSetup.Iso2Utf));
   Add(new cMenuEditBoolItem(tr("Spectrum Analyzer (music)"),  &GraphTFTSetup.enableSpectrumAnalyzer));

   asprintf(&buf, "---------------- %s ---------------------------------", tr("Dump Image"));
   Add(new cOsdItem(buf));
   free(buf);
   cList<cOsdItem>::Last()->SetSelectable(false);
   Add(new cMenuEditBoolItem(tr("Dump image to '/tmp/graphtftng.png'"), 
                                                               &GraphTFTSetup.DumpImage));
   Add(new cMenuEditIntItem(tr("Dump every [sec]"),            &GraphTFTSetup.DumpRefresh,0,600));

   asprintf(&buf, "---------------- %s ---------------------------------", tr("Snapshot"));
   Add(new cOsdItem(buf));
   free(buf);
   cList<cOsdItem>::Last()->SetSelectable(false);
   Add(new cMenuEditIntItem(tr("Snapshot width"),              &GraphTFTSetup.snapshotWidth, 1, 1920));
   Add(new cMenuEditIntItem(tr("Snapshot height"),             &GraphTFTSetup.snapshotHeight, 1, 1080));
   Add(new cMenuEditIntItem(tr("Snapshot JPEG Quality"),       &GraphTFTSetup.snapshotQuality, 0, 100));
   Add(new cMenuEditStrItem(tr("Snapshot path"),               GraphTFTSetup.snapshotPath, 
                            cGraphTFTSetup::sizePath,          trVDR(FileNameChars)));
   asprintf(&buf, "---------------- %s ---------------------------------", tr("FB/X Device"));
   Add(new cOsdItem(buf));
   free(buf);
   cList<cOsdItem>::Last()->SetSelectable(false);
   Add(new cMenuEditIntItem(tr("Width"),                       &GraphTFTSetup.width,1,1920));
   Add(new cMenuEditIntItem(tr("Height"),                      &GraphTFTSetup.height,1,1080));
   asprintf(&buf, "---------------- %s ---------------------------------", tr("FB Device"));
   Add(new cOsdItem(buf));
   free(buf);
   cList<cOsdItem>::Last()->SetSelectable(false);
   Add(new cMenuEditBoolItem(tr("Flip OSD"),                   &GraphTFTSetup.flipOSD));
   Add(new cMenuEditIntItem(tr("X Offset"),                    &GraphTFTSetup.xOffset,0,719));
   Add(new cMenuEditIntItem(tr("Y Offset"),                    &GraphTFTSetup.yOffset,0,575));
   Add(new cMenuEditIntItem(tr("Border to Width"),             &GraphTFTSetup.xBorder,0,575));
   Add(new cMenuEditIntItem(tr("Border to Height"),            &GraphTFTSetup.yBorder,0,719));
   asprintf(&buf, "---------------- %s ---------------------------------", tr("TCP Connection"));
   Add(new cOsdItem(buf));
   free(buf);
   cList<cOsdItem>::Last()->SetSelectable(false);
   Add(new cMenuEditIntItem(tr("JPEG Quality"),                &GraphTFTSetup.JpegQuality, 0, 100));

#ifdef WITH_TOUCH
   asprintf(&buf, "---------------- %s ---------------------------------", tr("touch Device"));
   Add(new cOsdItem(buf));
   free(buf);
   cList<cOsdItem>::Last()->SetSelectable(false);
   Add(new cMenuEditStrItem(tr("Device"),                     GraphTFTSetup.touchDevice, 
                            cGraphTFTSetup::sizePath,         trVDR(FileNameChars)));
#endif

   asprintf(&buf, "---------------- %s ---------------------------------", tr("Log"));
   Add(new cOsdItem(buf));
   free(buf);
   cList<cOsdItem>::Last()->SetSelectable(false);
   Add(new cMenuEditStraItem(tr("Log Device"),                &GraphTFTSetup.LogDevice, 4, logDevices));
   Add(new cMenuEditIntItem(tr("Log Level"),                  &GraphTFTSetup.Level,0,10));

#ifdef WITH_TOUCH
   setHelp();
#endif
}

cMenuSetupGraphTFT::~cMenuSetupGraphTFT()
{
   for (int i = 0; themeNames[i] && i < 100; i++)
      free(themeNames[i]);

   display->setCalibrate(off);
}

//***************************************************************************
// Set help Keys
//***************************************************************************

void cMenuSetupGraphTFT::setHelp()
{
   SetHelp(0, 
           display->isMode(cGraphTFTService::ModeCalibration) ? 0 : tr("Test"), 
           display->isMode(cGraphTFTService::ModeCalibration) ? tr("Stop") : tr("Calibrate"), 
           0);
}

//***************************************************************************
// Process Key
//***************************************************************************

eOSState cMenuSetupGraphTFT::ProcessKey(eKeys Key)
{
   eOSState state = cOsdMenu::ProcessKey(Key);

   if (state == osUnknown)
   {
      switch (Key)
      {
         case kOk:
         {
            Store();
            return osBack;
         }
         case kGreen:
         {
            // activate/deactivate callibration test mode

            display->switchCalibrate(cGraphTFTService::csTest);
            setHelp();

            return osContinue;
         }
         case kYellow:
         {
            // activate/deactivate callibration mode

            display->switchCalibrate();
            setHelp();

            return osContinue;
         } 

         default: break;
      }
   }
   else if (state == osBack)
   {
      display->setCalibrate(off);
   }

   return state;
}

//***************************************************************************
// Store
//***************************************************************************

void cMenuSetupGraphTFT::Store()
{
   GraphTFTSetup.Store();

   if (display)    
      display->setupChanged();
}
