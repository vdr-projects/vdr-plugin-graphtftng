/**
 *  GraphTFT plugin for the Video Disk Recorder 
 * 
 *  display.h - A plugin for the Video Disk Recorder
 *
 *  (c) 2004 Lars Tegeler, Sascha Volkenandt
 *  (c) 2006-2014 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 **/

#ifndef __GTFT_DISPLAY_H
#define __GTFT_DISPLAY_H

//***************************************************************************
// Includes
//***************************************************************************

#include <string>
#include <vector>
#include <algorithm>

#include <common.h>
#include <comthread.h>
#include <touchthread.h>
#include <renderer.h>
#include <theme.h>

#ifdef WITH_EPG2VDR
#  include "../vdr-plugin-epg2vdr/service.h"
#endif

#include <vdr/tools.h>
#include <vdr/thread.h>
#include <vdr/status.h>
#include <vdr/skins.h>

using std::string;
using std::vector;

//***************************************************************************
// Class cGraphTFTService
//***************************************************************************

class cGraphTFTService : public cThemeService
{
   public:

      enum ChannelType
      {
         ctRadio = 0,
         ctTv
      };

      enum { MaxTabs = 10 };
      
      enum CalibrationStart
      {
         csUnknown = na,
         csUpperLeft,
         csUpperRight,
         csLowerLeft,
         csLowerRight,
         csDone,
         csTest
      };

      enum DisplayMode 
      {
         ModeUnknown         = 0x00,

         ModeMask            = 0xF0,
         DisplayMask         = 0x0F,

         ModeNormal          = 0x10,
         ModeReplay          = 0x20,
         ModeMenu            = 0x30,

         NormalView          = 0x11,

         ReplayNormal        = 0x21,
         ReplayMP3           = 0x22,
         ReplayDVD           = 0x23,
         ReplayImage         = 0x24,

         MenuDefault         = 0x31,
         MenuMain            = 0x32,
         MenuSchedule        = 0x33,
         MenuChannels        = 0x34,
         MenuTimers          = 0x35,
         MenuRecordings      = 0x36,
         MenuSetup           = 0x37,
         MenuCommands        = 0x38,
         MenuWhatsOnElse     = 0x39,
         MenuWhatsOnNow      = 0x3a,
         MenuWhatsOnNext     = 0x3b,
         MenuSetupPage       = 0x3c,

         ModeCalibration     = 0x41,
         ModeCalibrationTest = 0x42
      };

      static int isModeNormal(DisplayMode mode)  { return (mode & ModeMask) == ModeNormal; }
      static int isModeReplay(DisplayMode mode)  { return (mode & ModeMask) == ModeReplay; }
      static int isModeMenu(DisplayMode mode)    { return (mode & ModeMask) == ModeMenu; }
};

//***************************************************************************
// Class cEventCopy
//***************************************************************************

class cEventCopy
{
   public:

      cEventCopy()    { initialized = no; title = 0; description = 0; shortText = 0; }
      ~cEventCopy()   { free(title); free(description); free(shortText); }

      void set(const cEvent* event)
      {
         if (!event)
         {
            initialized = no;
            return;
         }

         setEventID(event->EventID());
         setChannelID(event->ChannelID());

         setSeen(event->Seen());
         setTableId(event->TableID());
         setVersion(event->Version());
         setRunningStatus(event->RunningStatus());
         setTitle(event->Title());
         setShortText(event->ShortText());
         setDescription(event->Description());
         setParentalRating(event->ParentalRating());
         setStartTime(event->StartTime());
         setDuration(event->Duration());
         setVps(event->Vps());

         initialized = yes;
      }

      void set(const cEventCopy* event)
      {
         if (!event || event->isEmpty())
         {
            initialized = no;
            return;
         }

         setEventID(event->EventID());
         setChannelID(event->ChannelID());

         setSeen(event->Seen());
         setTableId(event->TableID());
         setVersion(event->Version());
         setRunningStatus(event->RunningStatus());
         setTitle(event->Title());
         setShortText(event->ShortText());
         setDescription(event->Description());
         setParentalRating(event->ParentalRating());
         setStartTime(event->StartTime());
         setDuration(event->Duration());
         setVps(event->Vps());
         
         initialized = yes;
      }

      int isEmpty() const                               { return !initialized; }
      void reset()                                      { initialized = no; }

      tEventID EventID() const                          { return eventId; }
      tChannelID ChannelID() const                      { return channelId; }
      time_t StartTime() const                          { return startTime; }
      time_t EndTime() const                            { return startTime + Duration(); }
      time_t Seen() const                               { return seen; }
      uchar TableID() const                             { return tableId; }
      bool IsRunning() const                            { return isRunning; }
      bool SeenWithin(int Seconds) const                { return time(0) - seen < Seconds; }
      time_t Vps() const                                { return vps; }
      uchar Version() const                             { return version; }
      uchar RunningStatus() const                       { return runningStatus; }
      uchar ParentalRating() const                      { return parentalRating; }
      int Duration() const                              { return duration; }
      const char* Title() const                         { return title; }
      const char* Description() const                   { return description; } 
      const char* ShortText() const                     { return shortText; }

   protected:

      void setEventID(tEventID EventId)                 { eventId = EventId; }
      void setChannelID(tChannelID ChannelId)           { channelId = ChannelId; }
      void setStartTime(time_t StartTime)               { startTime = StartTime; }
      void setRunning(bool IsRunning)                   { isRunning = IsRunning; }
      void setSeen(time_t Seen)                         { seen = Seen; }
      void setVps(time_t Vps)                           { vps = Vps; }
      void setTableId(uchar tid)                        { tableId = tid; }
      void setVersion(uchar Version)                    { version = Version; }
      void setRunningStatus(uchar RunningStatus)        { runningStatus = RunningStatus; }
      void setTitle(const char* Title)                  { free(title); title = strdup(Str::notNull(Title)); }
      void setDescription(const char* Description)      { free(description); description = strdup(Str::notNull(Description)); }
      void setParentalRating(uchar ParentalRating)      { parentalRating = ParentalRating; }
      void setShortText(const char* ShortText)          { free(shortText); shortText = strdup(Str::notNull(ShortText)); }
      void setDuration(int Duration)                    { duration = Duration; }

      int initialized;

      tChannelID channelId;
      tEventID eventId;
      time_t startTime;
      bool isRunning;
      time_t seen;
      time_t vps;
      uchar tableId;
      uchar version;
      uchar runningStatus;
      uchar parentalRating;
      int duration;

      char* title;
      char* description;
      char* shortText;
};

//***************************************************************************
// Class GraphTFT Display
//***************************************************************************

class cGraphTFTDisplay : public cStatus, cThread, public cGraphTFTService
{
   public:

      // object

      cGraphTFTDisplay(const char* aSyntaxVersion);
      ~cGraphTFTDisplay();

      // interface

      int init(const char* dev, int port, int startDetached);
      void setupChanged(int w = 0, int h = 0);
      void setCalibrate(int active, int state = csUnknown);
      void switchCalibrate(int state = csUnknown);
      int calibrateTouch(int x, int y);
      int setMode(DisplayMode mode, const char* menuName = 0, int force = false);
      int isMode(DisplayMode mode) { return mode == _mode; }
      void setCoverPath(const char* path) { _coverPath = Str::notNull(path); }
      void musicAddPlaylistItem(const char* item, int index = na);
      void setMusicPlayerState(cTftCS::MusicServicePlayerInfo* state);
      void setMusicPlayerHelpButtons(cTftCS::MusicServiceHelpButtons* buttons);
      cTouchThread* getTouchThread() { return touchThread; }

      Renderer* getRenderer()              { return renderer; }
      int attachXorg(const char* disp = 0) { if (!renderer) return fail; return renderer->attach(disp); }
      int detachXorg()                     { if (!renderer) return fail; return renderer->detach(); }

      // thread stuff

      bool Start()        { return cThread::Start(); }
      void Stop();
      bool Active() const { return _active; }
      cMutex* getMutex()  { return &_mutex; }

      void themesReloaded() { lastSection = 0; currentSection = 0; }

      // due to comThread destructor is called by vdr (inherited by cRemote)
      // don't delete comThread here!

      void clearComThread()   { comThread = 0; } 
      void clearTouchThread() { touchThread = 0; } 

      // event
      
      void mouseEvent(int x, int y, int button, int flag = ComThread::efNone, int data = 0);
      cDisplayItem* getItemAt(int x, int y);
      int processAction(cDisplayItem* p, string action, int step = 1);

      // display stuff

      void broadcast(int force = no);
      void triggerDump(const char* file, int width = na, int height = na);
      int display(string sectionName);
      void refresh();
      void clear();
      int updateGroup(int group);
      
      int getUnseenMails()
      {
         unsigned long mailCount = 0;

         if (cPluginManager::CallFirstService("MailBox-GetTotalUnseen-1.0", &mailCount))
            return mailCount;

         return na;
      }

      int hasNewMail()
      {
         int mailFlag = no;
         
         if (cPluginManager::CallFirstService("MailBox-HasNewMail-1.0", &mailFlag))
            return mailFlag;

         return no;
      }

      // some structures and types

      struct RdsInfo 
      {
          string text;
          string title;
          string artist;
      };

      class cTextList
      {
         public:

            cTextList() { it = 0; }

            virtual int count() = 0;

            virtual void clear() { reset(); };
            virtual void reset() { it = 0; }
            virtual void inc()   { it++; }
            virtual int iter()   { return it; }

            int isValid(int i = na) 
            { 
               i = i != na ? i : it;
               return  count() > 0 && i < count(); 
            }

         private:

            unsigned int it;
      };

      class TimerList : public cTextList
      {
         public:

            TimerList() : cTextList()  { runningCnt = 0; }

            void append(const cTimer* timer)
            {
               GtftTimerInfo info;

#ifdef WITH_EPG2VDR
               const cEpgTimer_Interface_V1* t = dynamic_cast<const cEpgTimer_Interface_V1*>(timer);
               info.running = t->hasState('R');
#else
               info.running = timer->Recording();
#endif
               
               info.title = timer->File();         // as default
               info.file = timer->File();
               info.start = timer->StartTime();
               info.stop = timer->StopTime();
               
               if (timer->Event())
                  info.title = timer->Event()->Title();

               timers.push_back(info);

               if (info.running) runningCnt++;
            }

            const char* firstRunning()
            {
               for (int i = 0; i < count(); i++)
                  if (timers[i].running)
                     return timers[i].title.c_str();

               return "";
            }

            time_t start()      { return !isValid() ? 0  : timers[iter()].start; }
            time_t stop()       { return !isValid() ? 0  : timers[iter()].stop; }
            const char* title() { return !isValid() ? "" : timers[iter()].title.c_str(); }
            const char* file()  { return !isValid() ? "" : timers[iter()].file.c_str(); }
            int running()       { return !isValid() ? 0  : timers[iter()].running; }
            
            int count()         { return timers.size(); }
            int countRunning()  { return runningCnt; }

            void clear()
            { 
               cTextList::clear(); 
               timers.clear(); 
               runningCnt = 0; 
            }

            void sort() { std::sort(timers.begin(), timers.end()); }

         protected:

            class GtftTimerInfo
            {
               public:
               string title;
               string file;
               time_t start;
               time_t stop;
               int running;    

               bool operator < (const GtftTimerInfo rhs) const { return start < rhs.start; }
            };
            
            int runningCnt;
            vector<GtftTimerInfo> timers;
      };

      class MusicPlayerInfo : public cTextList
      {
         public:
            
            MusicPlayerInfo() : cTextList() {}
            
            string filename;
            string artist;
            string album;
            string genre;
            string comment;
            string year;
            double frequence;
            int bitrate;
            string smode;
            bool loop;
            bool shuffle;
            bool shutdown;
            bool recording;
            int rating;
            int index;             // current index in tracklist
            int cnt;               // items in tracklist
            bool lyrics;
            bool copy;
            bool timer;
            string status;         // player status
            string currentTrack;
            vector<string> tracks; // tracklist (only the unplayed part!)
            
            // help buttons
            
            string red;
            string green;
            string yellow;
            string blue;
            
            int count()         { return tracks.size(); }

            const char* track() { return tracks[iter()].c_str(); }

            void clear()
            { 
               cTextList::clear(); 
               tracks.clear(); 
            }
      };
      
      // actual replay 

      struct ReplayInfo 
      {
         DisplayMode lastMode;
         string name;
         string fileName;
         cControl* control;
      };

      struct Calibration
      {
         string instruction;
         string info;
         int state;
         int cursorX;
         int cursorY;
         int lastX;
         int lastY;

         cTouchThread::CalibrationSetting settings;
      };

      struct MenuItem 
      {
         string text;
         string tabs[MaxTabs];
         int tabCount;
         int nextX;
         Ts::MenuItemType type;
         cEventCopy event;
         const cChannel* channel;
         const cRecording* recording;
         string recordingName;
      };
   
      struct MenuInfo 
      {
         DisplayMode lastMode;
         int currentRow;
         int currentRowLast;
         int visibleRows;
         int topRow;
         int lineHeight;
         int lineHeightSelected;
         string title;
         string current;
         string buttons[4];
         string text;
         int drawingRow;             // current drawing index
         vector<MenuItem> items;
         int charInTabs[MaxTabs+1];
      };

      // display contents

      cThemeSection* currentSection;
      cThemeSection* lastSection;
      ChannelType channelType;
      DisplayMode _mode;
      string _sectionName;

      int _eventsReady;
      bool _mute;
      int _volume;

      int _channel;
      const cChannel* _presentChannel;
      string _channelGroup;
      cEventCopy _presentEvent;
      cEventCopy _followingEvent;
      cEventCopy _event;
      string _coverPath;

      string _recording;
      ReplayInfo _replay;

      string _message;
      TimerList _timers;                  // the timers
      
      MenuInfo _menu;
      MusicPlayerInfo _music;             // special for Music Plugin
      RdsInfo _rds;                       // special for Radio Plugin
      Calibration calibration;
      int mouseX;
      int mouseY;
      int mouseKey;
      int touchMenu;
      uint64_t touchMenuHideAt;
      uint64_t touchMenuHideTime;

      // 

      int displayActive;
      int snapshotPending;
      int fdInotify;

   protected:

      // thread

      virtual void Action();

      // helper functions

      int wait(uint64_t updateIn);
      int meanwhile();
      void takeSnapshot();
      void updateProgram();
      void updateTimers();
      void updateChannel();
      void finalizeItemList();

      const char* getTabbedText(const char* s, int Tab);

      // status interface

      virtual void ChannelSwitch(const cDevice* Device, int ChannelNumber, bool LiveView);
      virtual void OsdSetEvent(const cEvent* event);
      virtual void OsdSetRecording(const cRecording* recording);
      virtual void OsdProgramme(time_t PresentTime, const char* PresentTitle, const char *PresentSubtitle, time_t FollowingTime, const char *FollowingTitle, const char *FollowingSubtitle);
      virtual void OsdChannel(const char* text);

      virtual void SetVolume(int Volume, bool Absolute);
      virtual void Replaying(const cControl *Control, const char *Name, const char *FileName, bool On);
      virtual void Recording(const cDevice *Device, const char *Name, const char *FileName, bool On);
      virtual void TimerChange(const cTimer *Timer, eTimerChange Change);
      virtual void OsdStatusMessage(const char *Message);
      virtual void OsdTitle(const char* Title);
      virtual void OsdItem(const char* Text, int Index);
      virtual void OsdEventItem(const cEvent* Event, const char* Text, int Index, int Count);
      virtual void OsdCurrentItem(const char *Text);
      virtual void OsdClear();
      virtual void OsdHelpKeys(const char* Red, const char* Green, const char* Yellow, const char* Blue);
      virtual void OsdTextItem(const char* Text, bool Scroll);
      virtual void OsdMenuDestroy();

#if defined _OLD_PATCH
      virtual void OsdMenuDisplay(const char* kind);
#else
      virtual void OsdMenuDisplay(eMenuCategory category);
#endif

   private:

      // renderer

      Renderer* renderer;
      ComThread* comThread;
      cTouchThread* touchThread;

      // thread control

      bool _active;
      cMutex _mutex;
      int needLock;
      cCondVar _doUpdate;
      int wakeup;
      char* userDumpFile;
      int userDumpWidth;
      int userDumpHeight;
      int triggerTimerUpdate;
      int triggerChannelUpdate;
      int triggerFinalizeItemList;

      //

      int forceNextDraw;
      int startedAt;
};

//***************************************************************************
#endif // __GTFT_DISPLAY_H
