/**
 *  GraphTFT plugin for the Video Disk Recorder 
 * 
 *  graphtftng.c
 *
 *  (c) 2004 Lars Tegeler, Sascha Volkenandt
 *  (c) 2006-2015 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 **/

#include <vdr/config.h>
#include <theme.h>
#include <getopt.h>

#include <service.h>
#include <graphtftng.h>
#include <span.h>

//***************************************************************************
// cGraphTFTMenu
//***************************************************************************
//***************************************************************************
// Object
//***************************************************************************

cGraphTFTMenu::cGraphTFTMenu(const char* title, cPluginGraphTFT* aPlugin)
{
   int i = 0;
   char* buf = 0;
   char** normalModes = Thms::theTheme->getNormalSections();

   SetMenuCategory(mcPluginSetup);
   SetTitle(title ? title : "");

   plugin = aPlugin;
   defMode = 0;
   dspActive = plugin->getDisplay()->displayActive;

   while (normalModes[i])
   {
      if (normalModes[i] == GraphTFTSetup.normalMode)
      {
         defMode = i;
         break;
      }

      i++;
   }
   originalDefMode = defMode;

   Clear();

   cOsdMenu::Add(new cOsdItem(tr("Reload themes")));
   cOsdMenu::Add(new cMenuEditBoolItem(tr("Refresh Display"), &dspActive));
   cOsdMenu::Add(new cMenuEditStraItem(tr("Normal Display"), 
                                       &defMode, 
                                       Thms::theTheme->getNormalSectionsCount(),
                                       normalModes));

   // user theme variables
   
   if (!Thms::theTheme->menuVariables.empty())
   {
      asprintf(&buf, "------ \t %s -----------------------------------------", 
               Thms::theTheme->getName().c_str());
      Add(new cOsdItem(buf));
      free(buf);
      cList<cOsdItem>::Last()->SetSelectable(false);

      map<string,string>::iterator iter;

      for (i = 0, iter = Thms::theTheme->menuVariables.begin(); 
           i < 50 && iter != Thms::theTheme->menuVariables.end(); 
           iter++, i++)
      {
         strncpy(variableValues[i], iter->second.c_str(), sizeVarMax);
         variableValues[i][sizeVarMax] = 0;

         cOsdMenu::Add(new cMenuEditStrItem(iter->first.c_str(), 
                                            variableValues[i],
                                            sizeVarMax, FileNameChars));
      }
   }

   SetHelp(tr("Snapshot"), 0, 0,0);

   Display();
}

cGraphTFTMenu::~cGraphTFTMenu()
{
}

//***************************************************************************
// Process Key
//***************************************************************************

eOSState cGraphTFTMenu::ProcessKey(eKeys key)
{
   eOSState state = cOsdMenu::ProcessKey(key);

	// RefreshCurrent();

   if (state != osUnknown)
      return state;

   switch (key)
   {
      case kRed:
      {
         plugin->getDisplay()->snapshotPending = yes;
         return osEnd;
      }
      case kOk:
      {
         Store();

         if (Current() == 0)
            plugin->loadThemes();

         return osEnd;
      }

      default:
         break;
   }

   return state;
}

void cGraphTFTMenu::Store()
{
   int i;

   if (defMode != originalDefMode)
   {
      GraphTFTSetup.normalMode = Thms::theTheme->getNormalSections()[defMode];
      GraphTFTSetup.storeNormalMode = true;
      GraphTFTSetup.originalNormalMode = "";
      plugin->SetupStore("normalMode", GraphTFTSetup.normalMode.c_str());
      Setup.Save();
   }

   plugin->getDisplay()->displayActive = dspActive;

   map<string,string>::iterator iter;

   for (i = 0, iter = Thms::theTheme->menuVariables.begin(); 
        i < 50 && iter != Thms::theTheme->menuVariables.end(); 
        iter++, i++)
   {
      iter->second = variableValues[i];
   }
}

//***************************************************************************
// cPluginGraphTFT
//***************************************************************************
//***************************************************************************
// Object
//***************************************************************************

cPluginGraphTFT::cPluginGraphTFT()
{
   display = 0;
   device = 0;
   port = 2039;
   startDetached = no;
}

cPluginGraphTFT::~cPluginGraphTFT()
{
   if (display)  delete display;

   free(device);
   Thms::theTheme = 0;
   themes.Clear();
}

//***************************************************************************
// Command Line Help
//***************************************************************************

const char *cPluginGraphTFT::CommandLineHelp()
{
   return "   -d <device>, --device=<device> set the output device\n"
          "                /dev/fb0\n"
          "                xorg:1.1\n"
          "                (default: none)\n"
          "   -p <port>, --port=<port> set the output port\n"
          "                (default: 2039)\n"
          "   -o, --detached start detached (offline) only supported for xorg device\n"
      ;
}

//***************************************************************************
// Process Args
//***************************************************************************

bool cPluginGraphTFT::ProcessArgs(int argc, char* argv[])
{
   int c;

   static option long_options[] = 
   {
      { "device",   required_argument, 0, 'd' },
      { "port",     required_argument, 0, 'p' },
      { "detached", no_argument, 0, 'o' },
      { 0, 0, 0, 0 }
   };

   // check the arguments

   while ((c = getopt_long(argc, argv, "d:p:o", long_options, 0)) != -1) 
   {
      switch (c)
      {
         case 'd': device = strdup(optarg); break;
         case 'p': port = atoi(optarg);     break;
         case 'o': startDetached = yes;     break;
         default:  tell(0, "Ignoring unknown argument '%s'", optarg);
      }
   }

   return true;
}

//***************************************************************************
// Initialize
//***************************************************************************

bool cPluginGraphTFT::Initialize()
{
   return true;
}

//***************************************************************************
// Start
//***************************************************************************

bool cPluginGraphTFT::Start()
{
   if (!device)
      asprintf(&device, "none");

   // init 

   GraphTFTSetup.setClient(this);

   display = new cGraphTFTDisplay(THEMEVERSION);

   logDevice = GraphTFTSetup.LogDevice;
   logLevel = GraphTFTSetup.Level;

   GraphTFTSetup.themesPath = ConfigDirectory() + string("/graphtftng/themes/");
   GraphTFTSetup.configPath = ConfigDirectory() + string("/graphtftng/");

   // load the thems

   if (loadThemes() != success)
      return false;

   if (display->init(device, port, startDetached) != success)
   {
      tell(0, "Error: Initializing failed, aborting!");

      return 0;
   }

   return 1;
}

//***************************************************************************
// Store
//***************************************************************************

void cPluginGraphTFT::Store()
{
   GraphTFTSetup.Store();
}

//***************************************************************************
// Load Themes
//***************************************************************************

int cPluginGraphTFT::loadThemes()
{
   char* buffer;
   FILE* p;
   cMutexLock lock(display->getMutex());

   // look for the themes in the config directory

   asprintf(&buffer, "find %s -follow -type f -name '*.theme' | sort", 
            GraphTFTSetup.themesPath.c_str());

   p = popen(buffer, "r");

   free(buffer);

   // first forget loaded themes

   themes.Clear();

   tell(0, "Loading themes");

   if (p)
   {
      char* s;
      cReadLine ReadLine;

      while ((s = ReadLine.Read(p)))
      {
         // Try to load the themes

         Thms::theTheme = new cGraphTFTTheme();

         tell(0, "Try loading theme '%s'", s);

         if (Thms::theTheme->load(s) != success)
         {
            delete Thms::theTheme;
            Thms::theTheme = 0;
            tell(0, "Ignoring invalid theme '%s'", s);

            continue;
         }

         tell(0, "Theme '%s' loaded", s);

         Thms::theTheme->check(THEMEVERSION);
         themes.Add(Thms::theTheme);
      }
   }

   pclose(p);

   // Have we found themes?

   if (!themes.Count())
   {
      tell(0, "Fatal: No themes found, aborting!");
      return fail;
   }

   Thms::theTheme = themes.getTheme(GraphTFTSetup.Theme);
   
   if (!Thms::theTheme)
      Thms::theTheme = themes.Get(0);

   tell(0, "Loaded %d themes", themes.Count());

   Thms::theTheme->activate(na); // display->fdInotify);

   tell(0, "Activated theme '%s'", Thms::theTheme->getName().c_str());

   display->setupChanged();
   display->themesReloaded();

   return success;
}

//***************************************************************************
// Main Menu Action
//***************************************************************************

cOsdObject* cPluginGraphTFT::MainMenuAction()
{
   return new cGraphTFTMenu(MainMenuEntry(), this);
}

//***************************************************************************
// Setup Menu
//***************************************************************************

cMenuSetupPage* cPluginGraphTFT::SetupMenu()
{
  return new cMenuSetupGraphTFT(display);
}

//***************************************************************************
// Setup Parse
//***************************************************************************

bool cPluginGraphTFT::SetupParse(const char* Name, const char* Value)
{
  return GraphTFTSetup.SetupParse(Name, Value);
}

//***************************************************************************
// Service
//***************************************************************************

bool cPluginGraphTFT::Service(const char* id, void* data)
{
   if (strcmp(id, SPAN_CLIENT_CHECK_ID) == 0)
   {
		if (GraphTFTSetup.enableSpectrumAnalyzer && data)
			*((cSpanService::Span_Client_Check_1_0*)data)->isActive = true;

		return true;
   }

   else if (strcmp(id, GRAPHTFT_TOUCH_EVENT_ID) == 0)
   {
      cTftCS::GraphTftTouchEvent* event;
      event = (cTftCS::GraphTftTouchEvent*)data;

      display->mouseEvent(event->x, event->y, event->button, event->flag);

      return true;
   }

   else if (strcmp(id, GRAPHTFT_CALIBRATION_ID) == 0)
   {
      display->setCalibrate(((cTftCS::GraphTftCalibration*)data)->activate);

      return true;
   }

   else if (strcmp(id, GRAPHTFT_COVERNAME_ID) == 0)
   {
      if (data)
      {
         display->setCoverPath(((cTftCS::MusicServiceCovername*)data)->name);

         if (display->isMode(cGraphTFTService::ReplayMP3))
            display->broadcast(yes);  
      }

      return true;
   }

   else if (strcmp(id, GRAPHTFT_PLAYLIST_ID) == 0)
   {
      if (data)
      {
         cTftCS::MusicServicePlaylist* p;
         p = (cTftCS::MusicServicePlaylist*)data;

         if (strcmp(p->item, "-EOL-") != 0)
            display->musicAddPlaylistItem(p->item, p->index);
         else
            display->musicAddPlaylistItem(tr("--- end of playlist ---"), p->index);

         if (p->index == p->count && display->isMode(cGraphTFTService::ReplayMP3))
            display->broadcast(yes);  
      }

      return true;
   }

   else if (strcmp(id, GRAPHTFT_STATUS_ID) == 0)
   {
      if (data)
      {
         cTftCS::MusicServicePlayerInfo* p;
         p = (cTftCS::MusicServicePlayerInfo*)data;

         tell(2, "Got state update from music plugin");
         display->setMusicPlayerState(p);

         if (display->isMode(cGraphTFTService::ReplayMP3))
            display->broadcast(yes);  
      }

      return true;
   }

   else if (strcmp(id, GRAPHTFT_HELPBUTTONS_ID) == 0)
   {
      if (data)
      {
         cTftCS::MusicServiceHelpButtons* p;
         p = (cTftCS::MusicServiceHelpButtons*)data;

         tell(0, "Got help buttons from music plugin");
         display->setMusicPlayerHelpButtons(p);

         if (display->isMode(cGraphTFTService::ReplayMP3))
            display->broadcast(yes);  
      }

      return true;
   }

   else if (strcmp(id, GRAPHTFT_INFO_ID) == 0)
   {
      if (data)
      {
         cTftCS::MusicServiceInfo* p;
         p = (cTftCS::MusicServiceInfo*)data;

         tell(0, "Got display info from music plugin");
         display->_music.currentTrack = p->info;

         if (display->isMode(cGraphTFTService::ReplayMP3))
            display->broadcast(yes);  
      }

      return true;
   }

   return false;
}

//***************************************************************************
// SVDRP Help Pages
//***************************************************************************

const char** cPluginGraphTFT::SVDRPHelpPages()
{
   static const char* HelpPages[] = 
   {
      "ACTIVE \n"
      "    Activate/Deactivate display refresh {ON|OFF}\n",
      "VIEW \n"
      "    Set the normal view to the specified section\n",
      "PREV \n"
      "    Set the normal view to the previous section\n",
      "NEXT \n"
      "    Set the normal view to the next section\n",
      "TVIEW \n"
      "    Set the normal view temporary to the specified section\n",
      "RVIEW \n"
      "    Reset the temporary 'normal view'\n",
      "REFRESH \n"
      "    force display refresh\n",
      "RELOAD \n"
      "    reload the actual theme\n",
      "ATTA \n"
      "    attach to xorg (if xorg is the selected output device)\n",
      "DETA \n"
      "    detach from xorg (if xorg is the selected output device)\n",
      "DISP <display> [width:height]\n"
      "    switch xorg display and optional the resolution (if xorg is the selected output device)\n",
      "DUMP <file> [<width> <height>]\n"
      "    dump the actual frame to <file> (specify a path with access rights for the vdr process)\n",
      0
   };

   return HelpPages;
}

//***************************************************************************
// SVDRP Command
//***************************************************************************

cString cPluginGraphTFT::SVDRPCommand(const char* Command, 
                                      const char* Option, 
                                      int& ReplyCode)
{
   const char* m = 0;

   if (!strcasecmp(Command, "ACTIVE")) 
   {
      if (Option && strcasecmp(Option, "ON") == 0) 
      {
         display->displayActive = yes;
         ReplyCode = 550;
         return "graphTFT display activated";
      }
      else if (Option && strcasecmp(Option, "OFF") == 0) 
      {
         display->displayActive = no;
         ReplyCode = 550;
         return "graphTFT display deactivated!";
      }
      else
      {
         ReplyCode = 901;
         return "Error: Unexpected option";
      }
   }

   else if (!strcasecmp(Command, "DETA"))
   {
      if (!display->getRenderer())
         return "Can't detach, no xorg device configured (use -d to setup the device)";

      if (display->detachXorg() != success)
         return "detache failed!";

      return "detached from xorg";
   }

   else if (!strcasecmp(Command, "ATTA"))
   {
      if (!display->getRenderer())
         return "Can't attach, no xorg device configured (use -d to setup the device)";

      if (display->attachXorg() != success)
         return "attach failed!";

      display->broadcast(yes);

      return "attached to xorg";
   }

   else if (!strcasecmp(Command, "DISP"))
   {
      char* opt = Option ? strdup(Option) : 0;
      int w = 0;
      int h = 0;
      char* p;

      if (!display->getRenderer())
         return "Can't set display, no xorg device configured (use -d to setup the device)";

      if (Str::isEmpty(opt))
         return "Missing option";

      if ((p = strchr(opt, ' ')))
      {
         *p = 0;
         p++;
         w = atoi(p);
         
         if ((p = strchr(p, ':')))
         {
            *p = 0;
            p++;
            h = atoi(p);
         }

         if (w && h)
            display->setupChanged(w, h);
      }

      if (display->detachXorg() != success)
         return "detache for display switch failed!";
      
      if (display->attachXorg(opt) != success)
         return "attach failed!";

      display->broadcast(yes);

      return "reattached to xorg";
   }

   else if (!strcasecmp(Command, "RELOAD"))
   {
      loadThemes();
      display->broadcast(yes);
      ReplyCode = 550;
      return "theme reloaded";
   }

   else if (!strcasecmp(Command, "REFRESH"))
   {
      display->broadcast(yes);
      ReplyCode = 550;
      return "display refreshed";
   }
   
   else if (!strcasecmp(Command, "PREV") || !strcasecmp(Command, "NEXT"))
   {
      if (!strcasecmp(Command, "NEXT"))
         GraphTFTSetup.normalMode = Thms::theTheme->nextNormalMode(GraphTFTSetup.normalMode.c_str());
      else
         GraphTFTSetup.normalMode = Thms::theTheme->prevNormalMode(GraphTFTSetup.normalMode.c_str());
      
      GraphTFTSetup.storeNormalMode = true;
      GraphTFTSetup.originalNormalMode = "";
      SetupStore("normalMode", GraphTFTSetup.normalMode.c_str());
      Setup.Save();
      display->broadcast(yes);
      
      ReplyCode = 550;
      return "Normal view changed";
   }
   else if (!strcasecmp(Command, "VIEW"))
   {
      if (Str::isEmpty(Option))
      {
         ReplyCode = 901;
         return "Error: Missing option";
      }

      if (!strcasecmp(Option, "TV") || !strcasecmp(Option, "RADIO"))
         Option = "Standard";

      if ((m = Thms::theTheme->getNormalMode(Option)))
      {
         GraphTFTSetup.normalMode = m;
         GraphTFTSetup.storeNormalMode = true;
         GraphTFTSetup.originalNormalMode = "";
      }
      else
      {
         ReplyCode = 901;
         return "Error: Unexpected option";
      }

      SetupStore("normalMode", GraphTFTSetup.normalMode.c_str());
      Setup.Save();
      display->broadcast(yes);

      tell(0, "Normal view now '%s'", GraphTFTSetup.normalMode.c_str());

      ReplyCode = 550;
      return "Normal view changed";
   }

   else if (!strcasecmp(Command, "TVIEW"))
   {
      if (!Str::isEmpty(Option) && (m = Thms::theTheme->getNormalMode(Option)))
      {
         if (GraphTFTSetup.originalNormalMode.empty())
            GraphTFTSetup.originalNormalMode = GraphTFTSetup.normalMode;

         GraphTFTSetup.normalMode = m;
         GraphTFTSetup.storeNormalMode = false;
      }
      else
      {
         ReplyCode = 901;
         return "Error: Unexpected option";
      }

      display->broadcast(yes);

      ReplyCode = 550;
      return "Normal view changed temporarily";
   }

   else if (!strcasecmp(Command, "RVIEW"))
   {
      if (!GraphTFTSetup.storeNormalMode && !GraphTFTSetup.originalNormalMode.empty())
      {
         GraphTFTSetup.normalMode = GraphTFTSetup.originalNormalMode;
         GraphTFTSetup.storeNormalMode = true;
         GraphTFTSetup.originalNormalMode = "";
      }
      else
      {
         ReplyCode = 901;
         return "Error: Unexpected option";
      }

      SetupStore("normalMode", GraphTFTSetup.normalMode.c_str());
      Setup.Save();
      display->broadcast(yes);

      ReplyCode = 550;
      return "Normal view reset";
   }

   else if (!strcasecmp(Command, "DUMP"))
   {
      char* file = strdup("/tmp/graphTFT.jpg");
      int width = na;
      int height = na;
      
      if (*Option)
      {
         char* p;

         free(file);
         file = strdup(Option);
         p = file;

         if ((p = strchr(p, ' ')))
         {
            *p = 0;
            width = atoi(++p);
         }
         
         if (p && (p = strchr(p, ' ')))
         {
            *p = 0;
            height = atoi(++p);
         }
      }
      
      display->triggerDump(file, width, height);

      free(file);

      ReplyCode = 550;
      return "Dump of image triggered";
   }

   return 0;
}

//***************************************************************************

VDRPLUGINCREATOR(cPluginGraphTFT)
