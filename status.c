/**
 *  GraphTFTng plugin for the Video Disk Recorder 
 * 
 *  status.c
 *
 *  (c) 2013-2013 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 * 
 **/

//***************************************************************************
// Includes
//***************************************************************************

#include <display.h>

//***************************************************************************
// Osd Channel Switch
//***************************************************************************

void cGraphTFTDisplay::ChannelSwitch(const cDevice* Device, int ChannelNumber, bool LiveView)
{
   tell(5, "ChannelSwitch on %p: %d", Device, ChannelNumber);

   if (LiveView 
       && _channel != ChannelNumber 
       && cDevice::CurrentChannel() != _channel) 
   {
      needLock = yes;
      cMutexLock lock(&_mutex);

      _rds.title.clear();
      _rds.artist.clear();
      _rds.text.clear();
      _channel = ChannelNumber;
      _channelGroup = "";

      _presentEvent.reset();
      _followingEvent.reset();
      _event.reset();
      _presentChannel = 0;

      triggerChannelUpdate = yes;

      updateGroup(groupChannel);
      broadcast();
   }
}

//***************************************************************************
// Osd Channel
//***************************************************************************

void cGraphTFTDisplay::OsdChannel(const char* text)
{
   tell(5, "OsdChannel: '%s'", text);

   _channelGroup = text;

   updateGroup(groupChannel);
   broadcast();
}

//***************************************************************************
// Osd Set Event
//***************************************************************************

void cGraphTFTDisplay::OsdSetEvent(const cEvent* event)
{
   tell(5, "OsdSetEvent: '%s'", event ? event->Title() : "");

   _event.set(event);
   updateGroup(groupChannel);
}

//***************************************************************************
// Osd Set Recording
//***************************************************************************

void cGraphTFTDisplay::OsdSetRecording(const cRecording* recording)
{
   tell(5, "OsdSetRecording: '%s'", recording ? recording->Title() : "");

   _recording = recording ? recording->FileName() : "";
}

//***************************************************************************
// Osd Programme
//***************************************************************************

void cGraphTFTDisplay::OsdProgramme(time_t PresentTime, const char* PresentTitle, 
                                    const char* PresentSubtitle, time_t FollowingTime, 
                                    const char* FollowingTitle, const char* FollowingSubtitle)
{
   needLock = yes;
	cMutexLock lock(&_mutex);

	if (PresentTitle)
		_rds.title = PresentTitle;
	else
		_rds.title = "";

	if (PresentSubtitle)
		_rds.artist = PresentSubtitle;
	else
		_rds.artist = "";

	broadcast();
}

//***************************************************************************
// Set Volume
//***************************************************************************

void cGraphTFTDisplay::SetVolume(int, bool) 
{
   tell(5, "SetVolume to %d, muted is %s", 
     cDevice::CurrentVolume(), 
     cDevice::PrimaryDevice()->IsMute() ? "true" : "false");

   if (_volume != cDevice::CurrentVolume()
       || _mute != cDevice::PrimaryDevice()->IsMute()) 
   {
      needLock = yes;
      cMutexLock lock(&_mutex);

      _volume = cDevice::CurrentVolume();
      _mute = cDevice::PrimaryDevice()->IsMute();

      updateGroup(groupVolume);
      broadcast(yes);
   }
}

//***************************************************************************
// Recording
//***************************************************************************

void cGraphTFTDisplay::Recording(const cDevice* Device, const char* Name, 
                                 const char* FileName, bool On) 
{
   tell(5, "Recording %s to %p", Name, Device);

   triggerTimerUpdate = yes;
   broadcast();
}

//***************************************************************************
// Timer Change
//***************************************************************************

void cGraphTFTDisplay::TimerChange(const cTimer *Timer, eTimerChange Change)
{
   tell(5, "TimerChange %s - %d ", Timer ? Timer->File() : "", Change);

   triggerTimerUpdate = yes;
   broadcast();
}

//***************************************************************************
// Replaying
//***************************************************************************

void cGraphTFTDisplay::Replaying(const cControl* Control, const char* Name,
                                 const char* FileName, bool On) 
{
   static const char* suffixes[] = { ".ogg", ".mpg", ".mpeg", ".mp3", ".avi", 0 };

   tell(5, "Replaying '%s' of '%s' file '%s'; control* is (%p)", 
        On ? "start" : "stop", 
        Name, Str::notNull(FileName),
        Control);

   needLock = yes;
   cMutexLock lock(&_mutex);

   if (On)
   {
      string::size_type i, n;

      if (!Name) Name = "";

      _replay.control = (cControl*)Control;
      _replay.lastMode = _mode;
      setMode(ReplayNormal);
      _replay.fileName = Str::notNull(FileName);
      // OsdSetRecording(Recordings.GetByName(Str::notNull(FileName)));

      if (strlen(Name) > 6 && Name[0]=='[' && Name[3]==']' && Name[5]=='(') 
      {
         for (i = 6; Name[i]; ++i) 
         {
            if (Name[i] == ' ' && Name[i-1] == ')')
               break;
         }

         if (Name[i]) 
         { 
            // replaying mp3

            const char* name = skipspace(Name + i);
            _replay.name = *name ? name : tr("Unknown title");
            setMode(ReplayMP3);
         }
      } 
      else if (strcmp(Name, "image") == 0)
      {
          if (!FileName)
             _replay.name = Name;
          else
              _replay.name = basename(FileName);

          setMode(ReplayImage);
      }
      else if (strcmp(Name, "DVD") == 0) 
      {
         _replay.name = Name;
         setMode(ReplayDVD);
      } 
      else if (strlen(Name) > 7) 
      {
         for (i = 1, n = 0; Name[i]; ++i) 
         {
            if (Name[i] == ' ' && Name[i-1] == ',' && ++n == 4)
               break;
         }

         if (Name[i]) 
         { 
            // replaying DVD

            const char *name = skipspace(Name + i);
            _replay.name = *name ? name : tr("Unknown title");
            replace(_replay.name.begin(), _replay.name.end(), '_', ' ');
            setMode(ReplayDVD);
         }
      }

      if (_mode == ReplayNormal) 
      {
         _replay.name = Name;

         if ((i = _replay.name.rfind('~')) != string::npos)
            _replay.name.erase(0, i + 1);
      }

      char* tmp;
      asprintf(&tmp, "%s", _replay.name.c_str());
      _replay.name = Str::allTrim(tmp);
      free(tmp);

      for (int l = 0; suffixes[l]; l++)
      {
         if ((i = _replay.name.rfind(suffixes[l])) != string::npos) 
            _replay.name.erase(i);
      }

      tell(5, "reformatted name is '%s'", _replay.name.c_str());
      updateGroup(groupReplay);
      broadcast();
   } 

   else 
   {
      _replay.control = 0;
      _replay.name = "";
      _replay.fileName = "";
 
      setMode(NormalView);
      broadcast();
   }
}

//***************************************************************************
// Osd Status Message
//***************************************************************************

void cGraphTFTDisplay::OsdStatusMessage(const char* Message) 
{
   tell(5, "OsdStatusMessage %s", Message);

   needLock = yes;
   cMutexLock lock(&_mutex);

   int force = _message != "";

   _message = Message ? Message : "";
   
   updateGroup(groupMessage);
   broadcast(force);
}

//***************************************************************************
// Osd Title
//***************************************************************************

void cGraphTFTDisplay::OsdTitle(const char* Title) 
{
   tell(5, "OsdTitle %s", Title);

   if (Title)
   {
      string::size_type pos;
      needLock = yes;
      cMutexLock lock(&_mutex);

      _message = "";
      _menu.title = Title;

      if ((pos = _menu.title.find('\t')) != string::npos)
         _menu.title.erase(pos);
   }
}

//***************************************************************************
// Osd Event Item (called after corresponding OsdItem!)
//***************************************************************************

void cGraphTFTDisplay::OsdEventItem(const cEvent* Event, const char* Text, 
                                    int Index, int Count)
{
   tell(5, "OsdEventItem '%s' at index (%d), count (%d)", 
        Event ? Event->Title() : "<null>", Index, Count);

   needLock = yes;
   cMutexLock lock(&_mutex);

   _eventsReady = no;

   if (_menu.items.size() <= (unsigned int)Index)
   {
      tell(1, "Got unexpected index %d, count is %ld", Index, _menu.items.size());
      return;
   }
   
   if (_menu.items[Index].text != Text)
      tell(0, "Fatal: Item index don't match!");
   else if (Event)
      _menu.items[Index].event.set(Event);

   if (Index == Count-1)
   {
      tell(3, "OsdEventItem() Force update due to index/count (%d/%d)", Index, Count);

  //    triggerFinalizeItemList = yes;
      updateGroup(groupMenu);
      broadcast();
   }
}

//***************************************************************************
// Osd Item
//***************************************************************************

void cGraphTFTDisplay::OsdItem(const char* Text, int Index) 
{
   MenuItem item;

   tell(5, "OsdItem '%s' at index (%d)", Text, Index);

   needLock = yes;
   cMutexLock lock(&_mutex);

   _eventsReady = no;

   if (!Text)
      Text = "";

   item.text = Text;
   item.type = itNormal;
   item.event.reset();
   item.channel = 0;
   item.tabCount = 0;
   int count = 1;
   char* pp;
   _menu.currentRow = na;
   
   const char* p = Text;
   
   while (*p)
   {
      if (*p == '\t')
         count++;
      p++;
   }
   
   _message = "";
   
   for (int i = 0; i < MaxTabs; ++i) 
   {
      const char* tp = getTabbedText(Text, i);
      
      if (tp)
      {
         char* tab;
         int len;
         
         tab = strdup(tp);
         len = strlen(Str::allTrim(tab));
         
         // bigger than last row ?
         
         if (_menu.charInTabs[i] < len)
            _menu.charInTabs[i] = len;
         
         // detect partingLine
         
         if (i == 0 && strstr(tab, "-----") && (pp = strchr(tab, ' ')))
         {
            char* e;
            *pp = 0;
            pp++;
            
            item.type = itPartingLine;
            
            if ((e = strchr(pp, ' ')))
               *e = 0;
            
            item.tabs[0] = Str::allTrim(pp);
         }
         
         else if (strstr(tab, "-----") && count < 2)
         {
            // arghdirector
            
            item.type = itPartingLine;
            item.tabs[0] = "";
         }
         else if (i == 1 && strstr(item.tabs[0].c_str(), "-----"))
         {
            char* p;
            
            // some special handling for lines of epgsearch plugin like
            // ---------------------------------- \t Die 24.10.2006 ------------------------
            
            item.type = itPartingLine;
            
            if ((p = strstr(tab, " ---")))
               *p = 0;
            
            if (!strstr(tab, "-----"))
               item.tabs[0] = Str::allTrim(tab);
            else
               item.tabs[0] = "";
            
            tell(5, "Detected parting line '%s'", item.tabs[0].c_str());
         }
         else
         {
            item.tabs[i] = tab;
            item.recordingName = tab;  // vermeintlicher name ...
         }
         
         tell(5, "tab (%d) '%s'", i, tp);
         item.tabCount++;
         free(tab);
      }
      else
         break;
   }
   
   tell(4, "adding item with (%d) tabs", item.tabCount);
   _menu.items.push_back(item);
}

//***************************************************************************
// Osd Current Item
//***************************************************************************

void cGraphTFTDisplay::OsdCurrentItem(const char* Text)
{
   if (!Text)
      Text = "";

   tell(3, "OsdCurrentItem '%s'", Text);

   string text = Text;

   if (text != _menu.current)
   {
      int i;

      needLock = yes;
      cMutexLock lock(&_mutex);

      _message = "";
      _menu.current = text;

      // lookup the item in the list and set the new current position

      for (i = 0; i < (int)_menu.items.size(); i++) 
      {
         if (_menu.items[i].text == _menu.current)
         {
            _menu.currentRow = i;

            updateGroup(groupMenu);
            broadcast();
            return ;
         }
      }

      // We don't have found the item, resume 
      //  old one with changed values ...

      if (_menu.currentRow < 0 || _menu.currentRow >= (int)_menu.items.size())
         return ;

      tell(5, "OsdCurrentItem: Text of current item changed to %s", Text);

// #if __GNUC__ < 3
//       MenuItem* item = _menu.items[_menu.currentRow];
// #else
//       MenuItem* item = _menu.items.at(_menu.currentRow);
// #endif

      _menu.items[_menu.currentRow].text = Text;

      for (int i = 0; i < MaxTabs; ++i) 
      {
         const char* tab = getTabbedText(Text, i);

         if (tab)
            _menu.items[_menu.currentRow].tabs[i] = tab;

         if (getTabbedText(Text, i+1) == 0)
            break;
      }

      updateGroup(groupMenu);
      broadcast();
   }
}

//***************************************************************************
// Osd Clear
//***************************************************************************

void cGraphTFTDisplay::OsdClear()
{
   string cg = _channelGroup;

   tell(0, "OsdClear");

   needLock = yes;
   cMutexLock lock(&_mutex);

   _menu.title = "";
   _menu.items.clear();
   _eventsReady = no;
   _menu.text = "";
   _menu.current = "";
   _channelGroup = "";

   _menu.charInTabs[0] = _menu.charInTabs[1] = _menu.charInTabs[2] = 
      _menu.charInTabs[3] = _menu.charInTabs[4] = _menu.charInTabs[5] = 0;

   if (_message != "" || cg != "")
   {
      _message = "";
      updateGroup(groupMessage);
      broadcast(yes);
   }
}

//***************************************************************************
// Osd Help Keys
//***************************************************************************

void cGraphTFTDisplay::OsdHelpKeys(const char* Red, const char* Green, 
                                   const char* Yellow, const char* Blue) 
{
   tell(5, "OsdHelpKeys: '%s', '%s', '%s', '%s'", Red, Green, Yellow, Blue);

   string red = Red ? Red : "";
   string green = Green ? Green : "";
   string yellow = Yellow ? Yellow : "";
   string blue = Blue ? Blue : "";

   if (_menu.buttons[0] != red ||
       _menu.buttons[1] != green ||
       _menu.buttons[2] != yellow ||
       _menu.buttons[3] != blue)
   {
      needLock = yes;
      cMutexLock lock(&_mutex);

      _menu.buttons[0] = red;
      _menu.buttons[1] = green;
      _menu.buttons[2] = yellow;
      _menu.buttons[3] = blue;
      
      updateGroup(groupButton);
      broadcast();
   }
}

//***************************************************************************
// Osd Text Item
//***************************************************************************

void cGraphTFTDisplay::OsdTextItem(const char* Text, bool Scroll) 
{
   tell(5, "OsdTextItem: '%s' scroll: %d", Text, Scroll);

   if (!Text)
      Text = "";

   needLock = yes;
   cMutexLock lock(&_mutex);
   
   _rds.text = Text;
   _menu.text = Text;
   broadcast();
}

//***************************************************************************
// Osd Menu Display
//***************************************************************************

#if defined _OLD_PATCH
void cGraphTFTDisplay::OsdMenuDisplay(const char* kind)
{
   tell(5, "OsdMenuDisplay - kind '%s'", kind);

   if (!isModeMenu(_mode)) 
      _menu.lastMode = _mode;
   
   setMode(ModeMenu, kind);
}
#else
void cGraphTFTDisplay::OsdMenuDisplay(eMenuCategory category)
{
   struct cCategorie
   {
      eMenuCategory category;
      const char* name;
   };

   cCategorie categories[] =
   {
      { mcUnknown,       "Menu" },

      { mcMain,          "MenuMain" },

      { mcSchedule,      "MenuSchedule" },
      { mcScheduleNow,   "MenuWhatsOnNow" },
      { mcScheduleNext,  "MenuWhatsOnNext" },

      { mcChannel,       "MenuChannels" },
      { mcChannelEdit,   "MenuEditChannel" } ,

      { mcTimer,         "MenuTimers" },
      { mcTimerEdit,     "MneuTimerEdit" },

      { mcRecording,     "MenuRecordings" },
      { mcRecordingInfo, "MenuRecording" },
      { mcRecordingEdit, "MenuRecordingEdit" },

      { mcPlugin,        "MenuPlugin" },
      { mcPluginSetup,   "MenuPluginSetup" },

      { mcSetup,         "MenuSetup" },
      { mcSetupOsd,      "MenuSetupOsd" },
      { mcSetupEpg,      "MenuSetupEpg" },
      { mcSetupDvb,      "MenuSetupDvb" },
      { mcSetupLnb,      "MenuSetupLnb" },
      { mcSetupCam,      "MenuSetupCam" },
      { mcSetupRecord,   "MenuSetupRecord" },
      { mcSetupReplay,   "MenuSetupReplay" },
      { mcSetupMisc,     "MenuSetupMisc" },
      { mcSetupPlugins,  "MenuSetupPlugins" },

      { mcCommand,       "MenuCommand" },
      { mcEvent,         "MenuEvent" },
      { mcText,          "MenuText" },
      { mcFolder,        "MenuFolder" },
      { mcCam,           "MenuCam" },

      { mcUndefined,     "Menu" }
   };

   const char* categoryName = "MenuUnknown";

   if (category > mcUndefined && category <= mcCam)
      categoryName = categories[category].name;

   tell(0, "OsdMenuDisplay - set category (%d) to '%s'", category, categoryName);

   if (!isModeMenu(_mode)) 
      _menu.lastMode = _mode;
   
   setMode(ModeMenu, categoryName);
}
#endif

//***************************************************************************
// Osd Menu Destroy
//***************************************************************************

void cGraphTFTDisplay::OsdMenuDestroy()
{
   tell(5, "OsdMenuDestroy");

   triggerTimerUpdate = yes;
   
   // all menues closed !

   if (isModeMenu(_mode))        // 'if' is needed for replay via extrecmenu
      setMode(_menu.lastMode);

   broadcast();
}
