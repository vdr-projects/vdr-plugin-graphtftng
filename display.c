/**
 *  GraphTFTng plugin for the Video Disk Recorder
 *
 *  display.c
 *
 *  (c) 2006-2013 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 **/

//***************************************************************************
// Includes
//***************************************************************************

#include <sys/inotify.h>

#include <unistd.h>

#include <setup.h>
#include <display.h>
#include <scan.h>

// the render devices

#include "fbrenderer.h"
#include "dmyrenderer.h"

#ifdef WITH_X
#  include "xrenderer.h"
#endif

//***************************************************************************
// Object
//***************************************************************************

cGraphTFTDisplay::cGraphTFTDisplay(const char* aSyntaxVersion)
{
   startedAt = time(0);

   // init renderer

   renderer = 0;

   // init thread stuff

   _active = no;
   comThread = 0;
   touchThread = 0;
   triggerTimerUpdate = no;
   triggerChannelUpdate = no;
   triggerFinalizeItemList = no;

   // init contents

   currentSection = 0;
   lastSection = 0;
   _eventsReady = no;
   _menu.charInTabs[0] = _menu.charInTabs[1] = _menu.charInTabs[2] =
      _menu.charInTabs[3] = _menu.charInTabs[4] =
      _menu.charInTabs[5] = _menu.charInTabs[6] =  0;
   _mode = NormalView;
   _sectionName = "";
   channelType = ctTv;
   _channel = 0;
   _channelGroup = "";
   _presentChannel = 0;
   _volume = cDevice::CurrentVolume();
   _mute = cDevice::PrimaryDevice()->IsMute();
   _replay.control = 0;
   _replay.name = "";
   _replay.fileName = "";
   _replay.lastMode = ModeUnknown;
   _menu.currentRowLast = na;
   _menu.currentRow = na;
   _menu.current = "";
   _menu.visibleRows = 0;
   _menu.topRow = na;
   _menu.lineHeight = na;
   _menu.drawingRow = na;
   displayActive = yes;
   forceNextDraw = yes;
   wakeup = no;
   userDumpFile = 0;
   userDumpWidth = 0;
   userDumpHeight = 0;
   needLock = no;

   _recording = "";
   _coverPath = "";

   _music.filename = "";
   _music.artist = "";
   _music.album = "";
   _music.genre = "";
   _music.comment = "";
   _music.year = na;
   _music.frequence = 0;
   _music.bitrate = 0;
   _music.smode = "";
   _music.index = 0;
   _music.cnt = 0;
   _music.status = "";
   _music.currentTrack = "";
   _music.loop = no;
   _music.shuffle = no;
   _music.shutdown = no;
   _music.recording = no;
   _music.rating = 0;
   _music.lyrics = no;
   _music.copy = no;
   _music.timer = no;

   // snapshot

   snapshotPending = no;

   // calibration stuff

   calibration.cursorX = 0;
   calibration.cursorY = 0;
   calibration.instruction = "";
   calibration.info = "";
   calibration.state = csUnknown;
   mouseX = 0;
   mouseY = 0;
   mouseKey = 0;
   touchMenu = 0;
   touchMenuHideTime = 0;
   touchMenuHideAt = 0;

   // initialize inotify

   fdInotify = inotify_init1(IN_NONBLOCK);

   if (fdInotify < 0)
   {
      fdInotify = na;
      tell(0, "Couldn't initialize inotify, %m");
   }
}

cGraphTFTDisplay::~cGraphTFTDisplay()
{
   Stop();

   if (touchThread) delete touchThread;
   if (comThread)   delete comThread;
   if (renderer)    delete renderer;

   if (fdInotify != na)
      close(fdInotify);
}

//***************************************************************************
// Init
//***************************************************************************

int cGraphTFTDisplay::init(const char* dev, int port, int startDetached)
{
   char* devName = 0;

   // create renderer

   if (strstr(dev, "/dev/fb"))
   {
      devName = strdup(dev);
      tell(0, "Create FB renderer");
      renderer = new FbRenderer(GraphTFTSetup.xOffset, GraphTFTSetup.yOffset,
                                GraphTFTSetup.width, GraphTFTSetup.height,
                                GraphTFTSetup.configPath, GraphTFTSetup.Iso2Utf,
                                Thms::theTheme->getDir());
   }

#ifdef WITH_X
   else if (strstr(dev, "xorg:"))
   {
      devName = strdup(dev + strlen("xorg"));
      tell(0, "Create X renderer at display '%s'", devName);
      renderer = new XRenderer(GraphTFTSetup.xOffset, GraphTFTSetup.yOffset,
                               GraphTFTSetup.width, GraphTFTSetup.height,
                               GraphTFTSetup.configPath, GraphTFTSetup.Iso2Utf,
                               Thms::theTheme->getDir());
   }
#endif

   else
   {
      tell(1, "No device configured, only graphtft-fe supported");

      renderer = new DummyRenderer(GraphTFTSetup.xOffset, GraphTFTSetup.yOffset,
                                   GraphTFTSetup.width, GraphTFTSetup.height,
                                   GraphTFTSetup.configPath, GraphTFTSetup.Iso2Utf,
                                   Thms::theTheme->getDir());
   }

   // apply renderer settings

   renderer->setBorder(GraphTFTSetup.xBorder, GraphTFTSetup.yBorder);
   renderer->setProperties(GraphTFTSetup.xOffset, GraphTFTSetup.yOffset,
                           Thms::theTheme->getWidth(), Thms::theTheme->getHeight(),
                           GraphTFTSetup.Iso2Utf, Thms::theTheme->getDir());

   renderer->setDevName(devName);

   // detached start only for xorg supported

   if (!startDetached || !strstr(dev, "xorg:"))
   {
      if (renderer->init(/*lazy*/ yes) != success)
         return fail;
   }
   else
   {
      tell(0, "Starting detached!");
   }


#ifdef WITH_TCPCOM
   // for comthread use theme width/height instead of display width/height

   comThread = new ComThread(this, Thms::theTheme->getWidth(), Thms::theTheme->getHeight());

   if (comThread->init(renderer, port) != success)
   {
      tell(0, "Can't establish tcp listener at port %d, aborting", port);

      return fail;
   }

   comThread->Start();
#endif

#ifdef WITH_TOUCH

   // touch thread ..

   touchThread = new cTouchThread(this);
   touchThread->setDevice(GraphTFTSetup.touchDevice);

   if (touchThread->open() != success)
   {
      tell(0, "Can't establish touch thread, touch panel not available!");
      delete touchThread;
      touchThread = 0;
   }
   else
   {
      touchThread->setSetting(&GraphTFTSetup.touchSettings);
      touchThread->Start();
   }

#endif

   // Show the start image

   cDisplayItem::setRenderer(renderer);
   cDisplayItem::setVdrStatus(this);
   setupChanged();

   Start();

   return success;
}

//***************************************************************************
// Broadcast
//***************************************************************************

void cGraphTFTDisplay::broadcast(int force)
{
   forceNextDraw = force ? force : forceNextDraw;
   wakeup = yes;
   _doUpdate.Broadcast();
}

//***************************************************************************
// Set Mode
//***************************************************************************

int cGraphTFTDisplay::setMode(DisplayMode mode, const char* menuName, int force)
{
   if (_mode == ModeCalibration && !force)
      return done;

   if (_mode != mode || (menuName && _sectionName != menuName))
   {
      _mode = mode;
      _sectionName = menuName ? menuName : "Menu";

      tell(0, "Mode is set to (0x%X), menu section to '%s'",
        _mode, _sectionName.c_str());

      broadcast();
   }

   return done;
}

//***************************************************************************
// Setup Changed
//***************************************************************************

void cGraphTFTDisplay::setupChanged(int w, int h)
{
   logDevice = GraphTFTSetup.LogDevice;
   logLevel = GraphTFTSetup.Level;

   if (comThread)
      comThread->setJpegQuality(GraphTFTSetup.JpegQuality);

   if (touchThread)
      touchThread->setDevice(GraphTFTSetup.touchDevice, yes);

   if (renderer)
   {
      renderer->flushCache();
      renderer->setBorder(GraphTFTSetup.xBorder, GraphTFTSetup.yBorder);
      renderer->setProperties(GraphTFTSetup.xOffset, GraphTFTSetup.yOffset,
                             Thms::theTheme->getWidth(), Thms::theTheme->getHeight(),
                             GraphTFTSetup.Iso2Utf,
                              Thms::theTheme->getDir());
      renderer->setFontPath(Thms::theTheme->getFontPath());

      // for X renderer set display size too, ignored by FB renderer!

      renderer->setDisplaySize(w ? w : GraphTFTSetup.width,
                               h ? h : GraphTFTSetup.height);
   }
}

//***************************************************************************
// Switch/Set Calibrate
//***************************************************************************

void cGraphTFTDisplay::switchCalibrate(int state)
{
   if (isMode(ModeCalibration))
      setCalibrate(off);
   else
      setCalibrate(on, state);
}

void cGraphTFTDisplay::setCalibrate(int active, int state)
{
   static int lastActive = no;
   static cGraphTFTService::DisplayMode lastMode = cGraphTFTDisplay::NormalView;
   static string lastSection = "";

   if (active != lastActive)
   {
      lastActive = active;
      tell(0, "Info: %s calibration mode", active ? "starting" : "stopping");
   }

   calibration.settings = GraphTFTSetup.touchSettings;

   if (state == csUnknown && touchThread)
      touchThread->setCalibrate(active);

   if (active)
   {
      // store actual mode

      lastMode = _mode;
      lastSection = _sectionName;
      setMode(ModeCalibration, 0, /*force*/ true);

      calibration.state = state;

      if (state == csUnknown)
      {
         calibration.settings.swapXY = no;
         calibration.info = "calibration started";

         if (touchThread)
            touchThread->resetSetting();
      }
      else
      {
         calibration.instruction = "verify by touching ...";
         calibration.info = "testing calibration";
      }

      calibrateTouch(0, 0);
   }
   else
   {
      setMode(lastMode, lastSection.c_str(), /*force*/ true);
   }

   broadcast(yes);
}

//***************************************************************************
// Music Plugin Interface
//***************************************************************************

void cGraphTFTDisplay::musicAddPlaylistItem(const char* item, int index)
{
   if (index == 0)
      _music.tracks.clear();

   _music.tracks.push_back(item);
}

void cGraphTFTDisplay::setMusicPlayerState(cTftCS::MusicServicePlayerInfo* p)
{
   if (!p) return;

   _music.filename = p->filename;
   _music.artist = p->artist;
   _music.album = p->album;
   _music.genre = p->genre;
   _music.comment = p->comment;
   _music.year = p->year > 0 ? Str::toStr(p->year) : "--";
   _music.frequence = p->frequence;
   _music.bitrate = p->bitrate;
   _music.smode = p->smode;
   _music.index = p->index;
   _music.cnt = p->count;
   _music.status = p->status;
   _music.currentTrack = p->currentTrack;
   _music.loop = p->loop;
   _music.shuffle = p->shuffle;
   _music.shutdown = p->shutdown;
   _music.recording = p->recording;
   _music.rating = p->rating;
   _music.lyrics = no;   // TODO
   _music.copy = no;     // TODO
   _music.timer = no;    // TODO
}

void cGraphTFTDisplay::setMusicPlayerHelpButtons(cTftCS::MusicServiceHelpButtons* p)
{
   _music.red = p->red;
   _music.green = p->green;
   _music.yellow = p->yellow;
   _music.blue = p->blue;
}

//***************************************************************************
// Stop
//***************************************************************************

void cGraphTFTDisplay::Stop()
{
   if (_active)
   {
      _active = no;
      broadcast();
      Cancel(3);
   }
}

//***************************************************************************
// Get Tabbed Text
//***************************************************************************

const char* cGraphTFTDisplay::getTabbedText(const char* str, int index)
{
   static char buffer[1000+TB];
   const char* a = str;
   const char* b = strchrnul(a, '\t');

   while (*b && index-- > 0)
   {
      a = b + 1;
      b = strchrnul(a, '\t');
   }

   if (!*b)
      return index <= 0 ? a : 0;

   unsigned int n = b - a;

   if (n >= sizeof(buffer))
      n = sizeof(buffer) - 1;

   strncpy(buffer, a, n);
   buffer[n] = 0;

   return buffer;
}

//***************************************************************************
// Action
//***************************************************************************

void cGraphTFTDisplay::Action()
{
   uint64_t updateIn;
   int n;

   tell(0,"GraphTFT plugin display thread started (pid=%d)", getpid());

   // display the start image

   renderer->image(Thms::theTheme->getStartImage().c_str(), 0,0,0,0);
   renderer->refresh();

   // update the timer list

   updateTimers();

   // and give the plugin time to collect some data AND give vdr some time to
   // finish initialization before acquiring the lock

   sleep(3);

   while (!Thms::theTheme->isInitialized())
      usleep(100000);

   _mutex.Lock();       // mutex gets ONLY unlocked when sleeping
   _active = yes;

   // main loop

   while (_active)
   {
      tell(3, "action loop");

      if (touchMenu)
      {
         if (touchMenuHideAt && msNow() > touchMenuHideAt-100)
            touchMenu = 0;

         forceNextDraw = yes;
      }

      if (isModeNormal(_mode))
      {
         // do the work ...

         if (GraphTFTSetup.normalMode == "Standard")
            n = display(channelType == ctTv ? "NormalTV" : "NormalRadio");
         else
            n = display("Normal" + GraphTFTSetup.normalMode);
      }
      else
      {
         switch (_mode)
         {
            case ReplayNormal:    n = display("ReplayNormal");  break;
            case ReplayMP3:       n = display("ReplayMP3");     break;
            case ReplayDVD:       n = display("ReplayDVD");     break;
            case ReplayImage:     n = display("ReplayImage");   break;
            case ModeCalibration: n = display("Calibration");   break;
            case ModeMenu:        n = display(_sectionName);    break;
            default:              n = display("NormalTV");
         }
      }

      updateIn = SECONDS(60);  // the default

      if (currentSection)
      {
         updateIn = currentSection->getNextUpdateTime() - msNow();

         // auto hide of touch menu

         if (touchMenu && touchMenuHideAt)
         {
            if ((touchMenuHideAt - msNow()) < updateIn)
               updateIn = touchMenuHideAt - msNow();

            tell(3, "Autohide scheduled in %ldms", updateIn);
         }
      }

      if (updateIn < 10)     // 10ms -> the minimum
         updateIn = 10;

      // can't calc this inline, due to a format string problem ... ?

      int s = updateIn/1000; int us = updateIn%1000;

      tell(2, "Displayed %d Items, schedule next "
           "update in %d,%03d seconds", n, s, us);

      wait(updateIn);

      // some data update requests ...

      if (triggerTimerUpdate)
         updateTimers();

      if (triggerChannelUpdate)
         updateChannel();

      if (triggerFinalizeItemList)
         finalizeItemList();

      // snapshot

      if (snapshotPending)
         takeSnapshot();
   }

   isyslog("GraphTFT plugin display thread ended (pid=%d)", getpid());
}

//***************************************************************************
// Wait
//***************************************************************************

int cGraphTFTDisplay::wait(uint64_t updateIn)
{
   uint64_t waitStep = updateIn > 100 ? 100 : updateIn;
   uint64_t waitUntil = updateIn + msNow();

   wakeup = no;

   while (msNow() < waitUntil && !wakeup)
   {
      _doUpdate.TimedWait(_mutex, waitStep);

      if (waitUntil-msNow() < waitStep)
         waitStep = waitUntil-msNow();

      meanwhile();
   }

   return success;
}

//***************************************************************************
// Meanwhile
//***************************************************************************

int cGraphTFTDisplay::meanwhile()
{
   static bool play = false, forward = false;
   static int speed = 0;

   bool aPlay, aForward;
   int aSpeed;

   // check some VDR states on changes

   if (_replay.control && _replay.control->GetReplayMode(aPlay, aForward, aSpeed))
   {
      if (aPlay != play || aSpeed != speed || aForward != forward)
      {
         play = aPlay; forward = aForward; speed = aSpeed;

         tell(3, "Trigger update of replay group (replay mode changed)");
         updateGroup(groupReplay);
         broadcast();
      }
   }
/*
   // check inotify

   if (fdInotify != na)
   {
      const int sizeNameMax = 255;
      const int sizeBuf = 10 * (sizeof(inotify_event) + sizeNameMax);

      char buffer[sizeBuf];
      inotify_event* event = 0;

      int bytes = read(fdInotify, buffer, sizeBuf);

      for (int pos = 0; pos < bytes; pos += sizeof(inotify_event) + event->len)
      {
         event = (inotify_event*)&buffer[pos];

         tell(0, "got %d bytes fron inotify, mask %d, len %d, name '%s'",
              bytes, event->mask, event->len, event->name ? event->name : "<null>");

         if (event->mask & IN_CREATE || event->mask & IN_MODIFY)
         {
            cDisplayItem* p = Thms::theTheme->inotifies[event->wd];

            tell(0, "inotify check event");

            if (p)
            {
               p->scheduleDrawIn(10);

               tell(0, "Got notification for '%s'", p->Path().c_str());
            }
         }
      }
   }
*/
   // process X events

   renderer->xPending();

   return done;
}

//***************************************************************************
// Take Snapshot
//***************************************************************************

void cGraphTFTDisplay::takeSnapshot()
{
   char* path = 0;
   char* file = 0;

   snapshotPending = no;

   // it's better to wait half a second, give the menu a chance to close ..

   _doUpdate.TimedWait(_mutex, 500);

   // tv view or replay running

   if (_replay.fileName.length())
   {
      const cRecording* replay;

#if defined (APIVERSNUM) && (APIVERSNUM >= 20301)
      tell(0, "lock for takeSnapshot()");
      LOCK_RECORDINGS_READ;
      const cRecordings* recordings = Recordings;
#else
      cRecordings* recordings = &Recordings;
#endif

      if ((replay = recordings->GetByName(_replay.fileName.c_str())) && replay->Info())
         asprintf(&file, "%s", replay->Info()->Title());
   }
   else
   {
      if (!_presentEvent.isEmpty())
         asprintf(&file, "%s", _presentEvent.Title());
   }

   if (!file)
   {
      Skins.Message(mtInfo, tr("Can't save snapshot, missing event information"));
      return ;
   }

   strreplace(file, '/', ' ');
   asprintf(&path, "%s/%s.jpg", GraphTFTSetup.snapshotPath, file);

   if (cDevice::PrimaryDevice()->GrabImageFile(path, yes,
                                               GraphTFTSetup.snapshotQuality,
                                               GraphTFTSetup.snapshotWidth,
                                               GraphTFTSetup.snapshotHeight))
      Skins.Message(mtInfo, tr("Snapshot saved"));
   else
      Skins.Message(mtInfo, tr("Error saving snapshot"));

   renderer->flushCache();
   free(path);
   free(file);
}

//***************************************************************************
// update Timers
//***************************************************************************

#ifdef WITH_EPG2VDR
#  include "../vdr-plugin-epg2vdr/service.h"
#endif

void cGraphTFTDisplay::updateTimers()
{
   needLock = yes;
   cMutexLock lock(&_mutex);

   tell(3, "Clearing internal timer list.");
   _timers.clear();

#ifdef WITH_EPG2VDR

   std::list<cEpgTimer_Interface_V1*>::iterator it;
   cPlugin* pEpg2Vdr = cPluginManager::GetPlugin("epg2vdr");
   cEpgTimer_Service_V1 data;

   if (!pEpg2Vdr)
      return;

   if (pEpg2Vdr->Service(EPG2VDR_TIMER_SERVICE, &data))
   {
      tell(0, "Got list with %ld timers from epg2vdr", data.epgTimers.size());

      for (it = data.epgTimers.begin(); it != data.epgTimers.end(); ++it)
      {
         tell(0, "Adding '%s' timer '%s' to list %s",
              (*it)->isLocal() ? "local" : "remote",
              (*it)->File(),
              (*it)->hasState('R') ? "timer is recording" : "");

         _timers.append(*it);

         delete (*it);
      }
   }

#else // WITH_EPG2VDR

#if defined (APIVERSNUM) && (APIVERSNUM >= 20301)
   tell(0, "lock for updateTimers()");
   LOCK_TIMERS_READ;
   const cTimers* timers = Timers;
#else
   const cTimers* timers = &Timers;
#endif

   for (const cTimer* timer = timers->First(); timer; timer = timers->Next(timer))
   {
      tell(3, "Adding timer '%s' to list %s",
           timer->File(), timer->Recording() ? "timer is regording" : "");

      _timers.append(timer);
   }

#endif // WITH_EPG2VDR

   _timers.sort();

   triggerTimerUpdate = no;

   updateGroup(groupRecording);
   broadcast();
}

//***************************************************************************
// Update Channel
//***************************************************************************

void cGraphTFTDisplay::updateChannel()
{
#if defined (APIVERSNUM) && (APIVERSNUM >= 20301)
   tell(0, "lock for updateChannel()");
   LOCK_CHANNELS_READ;
   _presentChannel = Channels->GetByNumber(_channel);
#else
   _presentChannel = Channels.GetByNumber(_channel);
#endif

   if (_presentChannel)
   {
      if (!isModeMenu(_mode))
      {
         switch (_presentChannel->Vpid())
         {
            case 0:
            case 1:
            case 0x1fff: channelType = ctRadio;  break;
            default:     channelType = ctTv;     break;
         }

         setMode(NormalView);
      }
   }

   triggerChannelUpdate = no;
}

//***************************************************************************
// Finalize Item List
//***************************************************************************

void cGraphTFTDisplay::finalizeItemList()
{
#if defined (APIVERSNUM) && (APIVERSNUM >= 20301)
   const cChannels* channels = 0;
   const cRecordings* recordings = 0;
   cStateKey stateKeyChannels;
   cStateKey stateKeyRecordings;

   tell(0, "trylock for finalizeItemList(CHANNELS)");

   if (!(channels = cChannels::GetChannelsRead(stateKeyChannels, 500)))
   {
      tell(0, "can't get lock for finalizeItemList(CHANNELS), retrying later");
      return ;
   }

   tell(0, "trylock for finalizeItemList(REGORDINGS)");

   if (!(recordings = cRecordings::GetRecordingsRead(stateKeyRecordings, 500)))
   {
      stateKeyChannels.Remove();
      tell(0, "can't get lock for finalizeItemList(CHANNELS), retrying later");
      return ;
   }
#else
   cChannels* channels = &Channels;
   cRecordings* recordings = &Recordings;
#endif

   cMutexLock lock(&_mutex);
   needLock = yes;
   for (string::size_type i = 0; i < _menu.items.size(); i++)
   {
      if (!_menu.items[i].event.isEmpty())
         _menu.items[i].channel = channels->GetByChannelID(_menu.items[i].event.ChannelID());

      if (!_menu.items[i].recording && _menu.items[i].recordingName.size())
         _menu.items[i].recording = recordings->GetByName(_menu.items[i].recordingName.c_str());
   }

#if defined (APIVERSNUM) && (APIVERSNUM >= 20301)
   stateKeyChannels.Remove();
   stateKeyRecordings.Remove();
#endif

   _eventsReady = yes;
   triggerFinalizeItemList = no;
}

//***************************************************************************
// Item At
//***************************************************************************

cDisplayItem* cGraphTFTDisplay::getItemAt(int x, int y)
{
   // first check foreground items ..

   for (cDisplayItem* p = currentSection->getItems()->First(); p;
        p = currentSection->getItems()->Next(p))
   {
      if (!p->Foreground() || !p->evaluateCondition())
         continue;

      if (p->OnClick() == "" && p->OnDblClick() == "" && p->OnUp() == "" && p->OnDown() == "")
         continue;

      if (x >= p->X() && x <= p->X()+p->Width() &&
          y >= p->Y() && y <= p->Y()+p->Height())
      {
         return p;
      }
   }

   // now the other ..

   for (cDisplayItem* p = currentSection->getItems()->First(); p;
        p = currentSection->getItems()->Next(p))
   {
      if (p->Foreground() || !p->evaluateCondition())
         continue;

      if (p->OnClick() == "" && p->OnDblClick() == "" && p->OnUp() == "" && p->OnDown() == "")
         continue;

      if (x >= p->X() && x <= p->X()+p->Width() &&
          y >= p->Y() && y <= p->Y()+p->Height())
      {
         return p;
      }
   }

   return 0;
}

//***************************************************************************
// Mouse Event
//***************************************************************************

void cGraphTFTDisplay::mouseEvent(int x, int y, int button, int flag, int data)
{
   static int whipeDiff = 0;

   if (!currentSection)
      return ;

   mouseX = x;
   mouseY = y;
   mouseKey = button;

   cMutexLock lock(&_mutex);

   cDisplayItem* p = getItemAt(x, y);

   if (p)
      tell(4, "Mouse action on item %d [%s] at (%d/%d)", p->Item(),
           p->Text().c_str(), x, y);

   if (isMode(ModeCalibration))
   {
      calibrateTouch(x, y);
      updateGroup(groupCalibrate);
      broadcast(yes);

      if (calibration.state < csTest)
         return ;
   }

   if (button == cGraphTftComService::mbWheelUp)
   {
      if (!p || p->OnUp() == "")
         cRemote::Put(cKey::FromString("Up"));
      else
         processAction(p, p->OnUp());
   }

   else if (button == cGraphTftComService::mbWheelDown)
   {
      if (!p || p->OnDown() == "")
         cRemote::Put(cKey::FromString("Down"));
      else
         processAction(p, p->OnDown());
   }

   else if (button == cGraphTftComService::mbRight)
   {
      cRemote::Put(cKey::FromString("Back"));
   }

   else if (button == cGraphTftComService::mbLeft)
   {
      if (!p)
         return ;

      if (flag & cGraphTftComService::efVWhipe)
      {
         tell(3, "vertical whipe of (%d) pixel", data);

         whipeDiff += data;

         int step = abs(whipeDiff) / p->WhipeRes();

         if (step)
         {
            tell(3, "do step of (%d)", step);

            if (whipeDiff < 0)
            {
               if (p->OnDown() != "")
                  processAction(p, p->OnDown(), step);
            }
            else
            {
               if (p->OnUp() != "")
                  processAction(p, p->OnUp(), step);
            }

            whipeDiff = whipeDiff % p->WhipeRes();
         }

         return ;
      }

      whipeDiff = 0;

      // menue navigation area ?

      if (p->Item() == itemMenuNavigationArea && !touchMenu &&
          _menu.lineHeight > 0)
      {
         int clickRow;
         int yOff = y - p->Y();
         int currentRowY = (_menu.currentRow - _menu.topRow) * _menu.lineHeight;

         if (yOff < currentRowY)
            clickRow = yOff / _menu.lineHeight + _menu.topRow;
         else if (yOff < currentRowY + _menu.lineHeightSelected)
            clickRow = _menu.currentRow;
         else
            clickRow = (yOff-_menu.lineHeightSelected) / _menu.lineHeight + _menu.topRow+1;

         if (_menu.currentRow < clickRow)        // down
            for (int i = _menu.currentRow; i < clickRow; i++)
               cRemote::Put(cKey::FromString("Down"));
         else if (_menu.currentRow > clickRow)   // up
            for (int i = _menu.currentRow; i > clickRow; i--)
               cRemote::Put(cKey::FromString("Up"));
      }

      // check if item with defined action

      if (p->OnClick() != "" && flag == cGraphTftComService::efNone)
      {
         if (p->OnClick().find("touchMenu") == 1)
         {
            char* val;
            char* val2;
            char* click;

            asprintf(&click, "%s", p->OnClick().c_str());

            // touch menu handling

            if ((val = strchr(click, ':')) && *(val++))
            {
               if ((val2 = strchr(val, ':')) && *(val2++)
                   && atoi(val) == touchMenu)
                  touchMenu = atoi(val2);
               else
                  touchMenu = atoi(val);
            }
            else
               touchMenu = !touchMenu;

            if (p->Delay())
               touchMenuHideTime = p->Delay();

            if (touchMenuHideTime)
               touchMenuHideAt = msNow() + touchMenuHideTime;

            tell(4, "touch menu switched to (%d) hide in (%ld) seconds",
                 touchMenu, touchMenuHideTime);

            free(click);
            broadcast(yes);
         }
         else
         {
            // no key, and not the 'touchMenu' command

            processAction(p, p->OnClick());
         }
      }

      if (p->OnDblClick() != "" && (flag & cGraphTftComService::efDoubleClick))
      {
         processAction(p, p->OnDblClick());
      }
   }
}

//***************************************************************************
// Process Action
//***************************************************************************

int cGraphTFTDisplay::processAction(cDisplayItem* p, string action, int step)
{
   string v;
   Scan scan(action.c_str());
   string name;
   eKeys key;
   int value = 0;
   string str;

   scan.eat();
   key = cKey::FromString(scan.lastIdent());

   tell(4, "Performing mouse action (%d) times, key '%s', %sfound in VDRs keytab",
        step, scan.all(), key != kNone ? "" : "not ");

   // first check if 'normal' key actions

   if (key != kNone)
   {
      for (int i = 0; i < abs(step); i++)
      {
         scan.reset();

         while (scan.eat() == success)
         {
            if ((key = cKey::FromString(scan.lastIdent())) != kNone)
               cRemote::Put(key);
         }
      }

      return success;
   }

   // perform special action on theme variable

   if (!scan.isIdent() || p->lookupVariable(scan.lastIdent(), v) != success)
   {
      tell(0, "Error: Invalid variable '%s' in '%s'",
           scan.lastIdent(), scan.all());
      return fail;
   }

   name = scan.lastIdent();

   if (scan.eat() != success || !scan.isOperator())
   {
      tell(0, "Error: Invalid operator '%s' in '%s'",
           scan.lastIdent(), scan.all());
      return fail;
   }

   // get the actual value of the variable

   value = atoi(v.c_str());

   // perform action on the value

   if (scan.hasValue("++"))
      value += step;
   else if (scan.hasValue("--"))
      value -= step;
   else if (scan.hasValue(":"))
   {
      int v1;

      if (scan.eat() != success || !scan.isNum())
      {
         tell(0, "Missing int value in '%s', ignoring", scan.all());
         return fail;
      }

      v1 = scan.lastInt();

      if (value == v1 && scan.eat() == success && scan.isOperator() && scan.hasValue(":"))
      {
         if (scan.eat() != success || !scan.isNum())
         {
            tell(0, "Missing second int value in '%s', ignoring", scan.all());
            return fail;
         }

         value = scan.lastInt();
      }
      else
      {
         value = v1;
      }
   }
   else
   {
      tell(0, "Unexpected operation in '%s', ignoring", scan.all());
      return fail;
   }

   tell(6, "Setting '%s' from (%s) to (%d)", name.c_str(), v.c_str(), value);
   p->setVariable(name.c_str(), value);
   broadcast(yes);

   return done;
}

//***************************************************************************
// Calibrate Touch Device
//***************************************************************************

int cGraphTFTDisplay::calibrateTouch(int x, int y)
{
   static double upperLeftX;
   static double upperLeftY;

   string s;
   int offset = 20;

   if (Thms::theTheme->lookupVar("calibrationFrameOffset", s) == success)
      offset = atoi(s.c_str());

   calibration.state++;

   switch (calibration.state)
   {
      case csUpperLeft:
      {
         calibration.instruction = "Click upper left corner";
         calibration.cursorX = offset;
         calibration.cursorY = offset;
         calibration.lastX = 0;
         calibration.lastY = 0;
         break;
      }
      case csUpperRight:
      {
         upperLeftX = x;
         upperLeftY = y;

         calibration.instruction = "Click upper right corner";
         calibration.cursorX = Thms::theTheme->getWidth() - offset;
         calibration.cursorY = offset;
         break;
      }
      case csLowerLeft:
      {
         // check for swap

         if (abs(calibration.lastY - y) > abs(calibration.lastX - x))
         {
            calibration.settings.swapXY = yes;
            if (touchThread) touchThread->resetSetting(yes);

            calibration.info = "detected flags (swapXY)";

            // restart calibration due to XY swap!

            calibration.state = csUnknown;
            calibrateTouch(0, 0);
            break;
         }

         calibration.settings.scaleX = ((double)(Thms::theTheme->getWidth() - 2*offset))
            / ((double)(x-calibration.lastX));

         calibration.instruction = "Click lower left corner";
         calibration.cursorX = offset;
         calibration.cursorY = Thms::theTheme->getHeight() - offset;

         break;
      }
      case csLowerRight:
      {
         calibration.settings.scaleY = ((double)(Thms::theTheme->getHeight()- 2*offset))
            / ((double)(y-calibration.lastY)); // upperLeftY

         calibration.settings.offsetX = (int)((((double)offset)/calibration.settings.scaleX) - upperLeftX);
         calibration.settings.offsetY = (int)((((double)offset)/calibration.settings.scaleY) - upperLeftY);
         calibration.settings.scaleWidth = Thms::theTheme->getWidth();
         calibration.settings.scaleHeight = Thms::theTheme->getHeight();

         calibration.instruction = "Click lower right corner";
         calibration.cursorX = Thms::theTheme->getWidth() - offset;
         calibration.cursorY = Thms::theTheme->getHeight() - offset;
         break;
      }
      case csDone:
      {
         if (touchThread)
         {
            touchThread->setSetting(&calibration.settings);
            touchThread->setCalibrate(off);
            GraphTFTSetup.touchSettings = calibration.settings;
            GraphTFTSetup.Store(yes);
         }

         tell(0, "Calibration done, offset (%d/%d), scale (%f/%f)",
              calibration.settings.offsetX, calibration.settings.offsetY,
              calibration.settings.scaleX, calibration.settings.scaleY);

         calibration.info = "verify by touching ...";
         calibration.instruction = "Calibration done";
         calibration.cursorX = x;
         calibration.cursorY = y;
      }
      case csTest:
      {
         calibration.info = "verify by touching ...";
         calibration.instruction = "Calibration done";
         calibration.cursorX = x;
         calibration.cursorY = y;

         break;
      }
      default:
      {
         calibration.cursorX = x;
         calibration.cursorY = y;
      }
   }

   tell(0, "Callibration step '%s'", calibration.instruction.c_str());

   calibration.lastX = x;
   calibration.lastY = y;

   return done;
}

//***************************************************************************
// Clear
//***************************************************************************

void cGraphTFTDisplay::clear()
{
   renderer->clear();
}

//***************************************************************************
// Display
//***************************************************************************

int cGraphTFTDisplay::display(string sectionName)
{
   int count = 0;

   LogDuration ld("cGraphTFTDisplay::display()");

   if (!displayActive)
      return 0;

   if (isModeNormal(_mode))
      updateProgram();

   else if (isModeMenu(_mode) && !Thms::theTheme->getSection(sectionName))
   {
      tell(0, "Info: Section faked to '%s' due to section '%s' not defined!",
        "Menu", sectionName.c_str());

      sectionName = "Menu";
   }

   if (!(currentSection = Thms::theTheme->getSection(sectionName)))
      return 0;

   // set/reset force flag

   cDisplayItem::setForce(forceNextDraw);
   forceNextDraw = no;

   if (cDisplayItem::getForce())
      tell(1, "Force draw of all items now");

   // section changed

   if (currentSection != lastSection)
   {
      tell(0, "Section changed from '%s' to '%s'",
        lastSection ? lastSection->getName().c_str() : "<none>",
        currentSection->getName().c_str());

      lastSection = currentSection;

      _menu.topRow = na;
      _menu.lineHeight = na;
      cDisplayItem::clearSelectedItem();

      clear();
      cDisplayItem::setForce(yes);
   }

   // for now ...

   if (isModeMenu(_mode))
   {
      cDisplayItem::setForce(yes);
      tell(3, "force due to menu section!");

      // reset x of menu items

      for (string::size_type i = 0; i < _menu.items.size(); i++)
         _menu.items[i].nextX = 0;
   }

   needLock = no;

   currentSection->updateVariables();
   updateGroup(groupVarFile);

   // draw items

   for (cDisplayItem* p = currentSection->getItems()->First();
        !needLock && p; p = currentSection->getItems()->Next(p))
   {
      if (!p->isForegroundItem() && !p->Foreground())
         count += p->refresh();
   }

   for (cDisplayItem* p = currentSection->getItems()->First();
        !needLock && p; p = currentSection->getItems()->Next(p))
   {
      if (p->isForegroundItem() || p->Foreground())
         count += p->refresh();
   }

   // refresh changed areas (only supported by X renderer)

   if (!isModeMenu(_mode))
   {
      for (cDisplayItem* p = currentSection->getItems()->First();
           !needLock && p; p = currentSection->getItems()->Next(p))
      {
         if (p->Changed())
            renderer->refreshArea(p->X(), p->Y(), p->Width(), p->Height());
      }
   }

   // update display

   if (!needLock && count)
      refresh();

   if (needLock)
      forceNextDraw = yes;

   return count;
}

//***************************************************************************
// Refresh
//***************************************************************************

void cGraphTFTDisplay::refresh()
{
   // LogDuration ld("cGraphTFTDisplay::refresh()");

   // refresh local display

   // renderer->refresh(cDisplayItem::getForce());

   renderer->refresh(isModeMenu(_mode));

   // refresh tcp client

   if (comThread)
      comThread->refresh();

   // dump to file ...

   if (GraphTFTSetup.DumpImage)
   {
      static int lastDumpAt = msNow();
      int lastDumpDiff = lastDumpAt ? msNow() - lastDumpAt : 0;

      if (lastDumpDiff > (GraphTFTSetup.DumpRefresh * 1000))
      {
         // dump

         lastDumpAt = msNow();
         renderer->dumpImage2File("/graphtftng.png", GraphTFTSetup.width, GraphTFTSetup.height);
      }
   }

   if (userDumpFile)
   {
      renderer->dumpImage2File(userDumpFile, userDumpWidth, userDumpHeight);
      free(userDumpFile);
      userDumpFile = 0;
   }
}

//***************************************************************************
// Update Programme
//***************************************************************************

void cGraphTFTDisplay::updateProgram()
{
#if defined (APIVERSNUM) && (APIVERSNUM >= 20301)
   tell(0, "lock for updateProgram(CHANNELS)");
   LOCK_CHANNELS_READ;
   const cChannel* channel = Channels->GetByNumber(_channel);
#else
   const cChannel* channel = Channels.GetByNumber(_channel);
#endif

   if (channel)
   {
      tell(1, "updateProgram for channel '%s'", channel->Name());
      cMutexLock lock(&_mutex);
      needLock = yes;

//       const cEvent* present = _presentEvent;
//       const cEvent* following = _followingEvent;

//       _presentEvent = _followingEvent = 0;

      _presentEvent.reset();
      _followingEvent.reset();

#if defined (APIVERSNUM) && (APIVERSNUM >= 20301)
      tell(0, "lock for updateProgram(SCHEDULES)");
      LOCK_SCHEDULES_READ;
      const cSchedules* schedules = Schedules;
#else
      cSchedulesLock schedulesLock;
      const cSchedules* schedules = (cSchedules*)cSchedules::Schedules(schedulesLock);
#endif

      if (schedules)
      {
         const cSchedule *schedule = schedules->GetSchedule(channel->GetChannelID());

         if (schedule)
         {
            _presentEvent.set(schedule->GetPresentEvent());
            _followingEvent.set(schedule->GetFollowingEvent());
         }
      }

//      if (present != _presentEvent || following != _followingEvent)
      {
         updateGroup(groupChannel);
         broadcast();
      }
   }
}

//***************************************************************************
// Trigger Dump
//***************************************************************************

void cGraphTFTDisplay::triggerDump(const char* file, int width, int height)
{
   userDumpWidth  = width  == na ? GraphTFTSetup.width  : width;
   userDumpHeight = height == na ? GraphTFTSetup.height : height;

   free(userDumpFile);
   userDumpFile = strdup(file);

   wakeup = yes;
   _doUpdate.Broadcast();
}

int cGraphTFTDisplay::updateGroup(int group)
{
   if (!currentSection)
      return ignore;

   return currentSection->updateGroup(group);
}
