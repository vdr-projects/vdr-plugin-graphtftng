/*
 *  GraphTFT plugin for the Video Disk Recorder 
 *
 * graphtftng.h
 *
 * (c) 2006-2016 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 */

#ifndef __GTFT__H__
#define __GTFT__H__

//***************************************************************************
// Includes
//***************************************************************************

#include <vdr/plugin.h>
#include <vdr/config.h>

#include "common.h"
#include "setup.h"
#include "display.h"
#include "HISTORY.h"

//***************************************************************************
// General
//***************************************************************************

static const char* DESCRIPTION    = trNOOP("VDR OSD on TFT");
static const char* MAINMENUENTRY  = trNOOP("Graph-TFTng");

class cPluginGraphTFT;

//***************************************************************************
// Class GraphTFTMenuItem
//***************************************************************************

class cGraphTFTMenuItem : public cOsdItem
{
   public:
   
      cGraphTFTMenuItem(const char* aTitle)
         : cOsdItem(aTitle) {}
};

//***************************************************************************
// Class GraphTFTMenu
//***************************************************************************

class cGraphTFTMenu : public cMenuSetupPage
{
   public:

      enum Size
      {
         sizeVarMax = 100
      };

      cGraphTFTMenu(const char* title, cPluginGraphTFT* aPlugin);
      virtual ~cGraphTFTMenu();
      
      virtual eOSState ProcessKey(eKeys key);

      const char* MenuKind() { return "MenuSetupPage"; }

   protected:

      void Store();

      cPluginGraphTFT* plugin;
      int defMode;
      int originalDefMode;
      int dspActive;
      char variableValues[50][sizeVarMax+TB];
};

//***************************************************************************
// Class cPluginGraphTFT
//***************************************************************************

class cPluginGraphTFT : public cPlugin, cThemeService
{
   public:

      cPluginGraphTFT();
      ~cPluginGraphTFT();

      const char* Version()          { return VERSION; }
      const char* Description()      { return tr(DESCRIPTION); }
      const char* CommandLineHelp();

      bool ProcessArgs(int argc, char* argv[]);
      bool Initialize();
      bool Start();
      void Store();
      void Housekeeping() {}

      const char* MainMenuEntry()    
      { return GraphTFTSetup.HideMainMenu ? 0 : tr(MAINMENUENTRY); }

      cOsdObject* MainMenuAction();

      cMenuSetupPage* SetupMenu();
      bool SetupParse(const char* Name, const char* Value);
      cGraphTFTDisplay* getDisplay() { return display; }

      bool Service(const char* Id, void* Data);
      const char** SVDRPHelpPages();
      cString SVDRPCommand(const char* Command, const char* Option, int& ReplyCode);

      int loadThemes();

   private:

      cGraphTFTDisplay* display;
      char* device;
      int startDetached;
      int port;
};

//***************************************************************************
#endif  // __GTFT__H__
