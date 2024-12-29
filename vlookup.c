/*
 *  GraphTFT plugin for the Video Disk Recorder 
 *
 * vlookup.c
 *
 * (c) 2007-2015 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 */

#include <display.h>
#include <scraper2vdr.h>

//***************************************************************************
// Copied vorm VDR code!
//***************************************************************************

#define FULLMATCH 1000

eTimerMatch Matches(const cTimer* ti, const cEventCopy* Event)
{
   if (ti->HasFlags(tfActive) && ti->Channel()->GetChannelID() == Event->ChannelID()) 
   {
      int overlap = 0;
      bool UseVps = ti->HasFlags(tfVps) && Event->Vps();
      
      if (UseVps)
         overlap = (ti->StartTime() == Event->Vps()) ? FULLMATCH + (Event->IsRunning() ? 200 : 100) : 0;
      
      if (!overlap) 
      {
         if (ti->StartTime() <= Event->StartTime() && Event->EndTime() <= ti->StopTime())
            overlap = FULLMATCH;
         else if (ti->StopTime() <= Event->StartTime() || Event->EndTime() <= ti->StartTime())
            overlap = 0;
         else
            overlap = (std::min(ti->StopTime(), Event->EndTime()) - std::max(ti->StartTime(), Event->StartTime())) * FULLMATCH / std::max(Event->Duration(), 1);
      }
      
      if (UseVps)
         return overlap > FULLMATCH ? tmFull : tmNone;
      
      return overlap >= FULLMATCH ? tmFull : overlap > 0 ? tmPartial : tmNone;
   }
   
   return tmNone;
}

#if VDRVERSNUM < 10733
   enum eTimerMatch { tmNone, tmPartial, tmFull };
#endif

const cTimer* getTimerMatch(const cTimers* timers, const cEventCopy* event, eTimerMatch* Match)
{
   const cTimer* t = 0;
   eTimerMatch m = tmNone;
   
   for (const cTimer* ti = timers->First(); ti; ti = timers->Next(ti)) 
   {
      eTimerMatch tm = Matches(ti, event);
      
      if (tm > m) 
      {
         t = ti;
         m = tm;
         
         if (m == tmFull)
            break;
      }
   }
   
   if (Match)
      *Match = m;
   
   return t;
}

//***************************************************************************
// Variable of Group
//***************************************************************************

const char* variableGroup(const char* value, const char* group, const char*& var)
{
   if (!group)
      return 0;

   if (strncasecmp(value, group, strlen(group)) == 0)
   {
      var = value + strlen(group);
      return group;
   }

   return 0;
}

//***************************************************************************
// Lookup Variable
//***************************************************************************

int cDisplayItem::lookupVariable(const char* name, string& value, const char* fmt)
{
   const char* p;
   int status;

   value = "";
   p = variable(name, fmt, status);

   if (p)
   {
      value = p;
      return success;
   }

   if (status == success)
      return fail;

   // nothing found ... -> check theme for global variable

   if (cThemeItem::lookupVariable(name, value, fmt) == success)
   {
      if (fmt && (strcasestr(name, "TIME") || strcasestr(name, "DATE")))
      {
         char buf[100];

         if (formatDateTime(atol(value.c_str()), fmt, buf, sizeof(buf)))
            value = buf;
      }

      return success;
   }

   // variable unknown :(

   tell(0, "Unexpected variable '%s'", name);

   return fail;
}

//***************************************************************************
// Variable
//***************************************************************************

const char* cDisplayItem::variable(const char* name, const char* fmt, int& status)
{
   static char buf[1000];
   const char* p;
   int total, current;
   const char* var;
   const char* group;

   status = success;

   if ((var = strchr(name, '.')))
   {
      cDisplayItem* item = 0;
      char id[50+TB];
      
      snprintf(id, 50, "%.*s", (int)(var-name), name);
      var++;

      if (!(item = section->getItemById(id)))
      {
         // no item with this id found -> should be a reference to a variable file,
         //  will processed by 'cThemeItem::lookupVariable'

         status = fail;

         return 0;
      }

      tell(4, "found item '%s' (%s), lookup '%s'", item->Id().c_str(), name, var);

      if (strcasecmp(var, "X") == 0)
         return Str::toStr(item->lastX);
      if (strcasecmp(var, "Y") == 0)
         return Str::toStr(item->lastY);
      if (strcasecmp(var, "Width") == 0)
         return Str::toStr(item->lastWidth);
      if (strcasecmp(var, "Height") == 0)
         return Str::toStr(item->lastHeight);
      else
         tell(0, "Unexpected theme variable %s in %s", var, name);

      return "0";
   }

   if ((group = variableGroup(name, "music", var)))
   {
      p = musicVariable(group, var, fmt, status);

      if (status == success)
         return p;
   }

   else if ((group = variableGroup(name, "recording", var))
            || (group = variableGroup(name, "replay", var))
            || (group = variableGroup(name, "rowRecording", var))
            || (group = variableGroup(name, "selectedRowRecording", var)))
   {
      const cRecording* recording = 0;

#if defined (APIVERSNUM) && (APIVERSNUM >= 20301)
      const cRecordings* recordings;
      {
      LOCK_RECORDINGS_READ;
      recordings = Recordings;
      }
#else
      cRecordings* recordings = &Recordings;
#endif

      if (strcmp(group, "recording") == 0)
         recording = recordings->GetByName(vdrStatus->_recording.c_str());
      else if (strcmp(group, "replay") == 0)
         recording = recordings->GetByName(vdrStatus->_replay.fileName.c_str());
      else if (strcmp(group, "rowRecording") == 0)
         recording = vdrStatus->_menu.drawingRow == na ?
            0 : vdrStatus->_menu.items[vdrStatus->_menu.drawingRow].recording;
      else if (strcmp(group, "selectedRowRecording") == 0)
         recording = vdrStatus->_menu.currentRow == na ?
            0 : vdrStatus->_menu.items[vdrStatus->_menu.currentRow].recording;
      
      // recording variables

      if (!recording && strcmp(group, "replay") != 0)
      {
         tell(0, "Missing '%s' info, can't lookup variable for '%s'", 
              name, vdrStatus->_recording.c_str());

         return 0;
      }

      if (vdrStatus->_replay.control)
      {
         if (strcasecmp(var, "Speed") == 0)
            return Str::toStr(replayModeValue(rmSpeed));
         else if (strcasecmp(var, "Play") == 0)
            return Str::toStr(replayModeValue(rmPlay));
         else if (strcasecmp(var, "Forward") == 0)
            return Str::toStr(replayModeValue(rmForward));
         else if (strcasecmp(var, "Current") == 0)
         {
            vdrStatus->_replay.control->GetIndex(current, total);
            return formatDateTime((current ? current : 1) / (int)vdrStatus->_replay.control->FramesPerSecond(),
                                  fmt, buf, sizeof(buf), yes);
         }
         else if (strcasecmp(var, "Total") == 0)
         {
            vdrStatus->_replay.control->GetIndex(current, total);
            return formatDateTime((total ? total : 1) / (int)vdrStatus->_replay.control->FramesPerSecond(), 
                                  fmt, buf, sizeof(buf), yes);
         }
         else if (strcasecmp(var, "RawCurrent") == 0)
         {
             vdrStatus->_replay.control->GetIndex(current, total);
             return Str::toStr(current);
         }
         else if (strcasecmp(var, "RawTotal") == 0)
         {
             vdrStatus->_replay.control->GetIndex(current, total);
             return Str::toStr(total);
         }
      }

      if (recording)
      {
         if (strcasecmp(var, "Title") == 0)
            return formatString(Str::notNull(recording->Info()->Title()), fmt, buf, sizeof(buf));
         else if (strcasecmp(var, "Path") == 0)
            return formatString(Str::notNull(recording->FileName()), fmt, buf, sizeof(buf));
         else if (strcasecmp(var, "Time") == 0)
            return formatDateTime(recording->Start(), fmt, buf, sizeof(buf));
         else if (strcasecmp(var, "EventID") == 0)
            return Str::toStr(!recording->Info() ? na : (int)recording->Info()->GetEvent()->EventID());
         else if (strcasecmp(var, "SubTitle") == 0)
            return Str::notNull(recording->Info()->ShortText());
         else if (strcasecmp(var, "Description") == 0)
            return  Str::notNull(recording->Info()->Description());
         else if (strcasecmp(var, "Channel") == 0)
            return formatString(Str::notNull(recording->Info()->ChannelName()), fmt, buf, sizeof(buf));
         else if (strcasecmp(var, "Banner") == 0)
         {
            string mediaPath;
            string posterPath;
            
            if (getScraperMediaPath(0, recording, mediaPath, posterPath) == success)
               tell(5, "Got banner path '%s'", mediaPath.c_str());
            
            return mediaPath.c_str();
         }
         else if (strcasecmp(var, "Poster") == 0)
         {
            string mediaPath;
            string posterPath;
            
            if (getScraperMediaPath(0, recording, mediaPath, posterPath) == success)
               tell(5, "Got poster path '%s'", posterPath.c_str());
            
            return posterPath.c_str();
         }
      }

      else
      {
         if (strcasecmp(var, "Title") == 0)
            return formatString(Str::notNull(vdrStatus->_replay.name.c_str()), fmt, buf, sizeof(buf));
         else if (strcasecmp(var, "EventID") == 0)
            return Str::toStr(na);
         else if (strcasecmp(var, "SubTitle") == 0)
            return "";
         else if (strcasecmp(var, "Channel") == 0)
            return "";
         else if (strcasecmp(var, "Path") == 0)
            return formatString(Str::notNull(vdrStatus->_replay.fileName.c_str()), fmt, buf, sizeof(buf));
         else if (strcasecmp(var, "Time") == 0)
            return "";
         else if (strcasecmp(var, "Description") == 0)
            return "no details available";
      }
   }

   else if ((group = variableGroup(name, "volume", var)))
   {
      if (strcasecmp(var, "Mute") == 0)
         return Str::toStr(vdrStatus->_mute);
      else if (strcasecmp(var, "Level") == 0)
         return Str::toStr(vdrStatus->_volume);
   }

   else if ((group = variableGroup(name, "event", var)) 
            || (group = variableGroup(name, "following", var))
            || (group = variableGroup(name, "present", var))
            || (group = variableGroup(name, "rowEvent", var))
            || (group = variableGroup(name, "selectedRowEvent", var)))
   {
      tell(5, "lookup variable '%s' of group '%s'", var, group);

      eTimerMatch timerMatch = tmNone;
      cEventCopy* event = 0;
      const cChannel* channel = 0;

#if defined (APIVERSNUM) && (APIVERSNUM >= 20301)
      const cChannels* channels;
      {
      LOCK_CHANNELS_READ;
      channels = Channels;
      }
#else
      cChannels* channels = &Channels;
#endif

      if (strcmp(group, "event") == 0)
      {
         if (!vdrStatus->_event.isEmpty())
         {
            event = &vdrStatus->_event;
            channel = channels->GetByChannelID(event->ChannelID());
         }
      }
      else if (strcmp(group, "present") == 0)
      {
         if (!vdrStatus->_presentEvent.isEmpty())
           event = &vdrStatus->_presentEvent;

         channel = vdrStatus->_presentChannel;
      }
      else if (strcmp(group, "following") == 0)
      {
         if (!vdrStatus->_followingEvent.isEmpty())
            event = &vdrStatus->_followingEvent;

         channel = vdrStatus->_presentChannel;
      }
      else if (strcmp(group, "rowEvent") == 0)
      {
         if (vdrStatus->_menu.drawingRow != na)
            event = &vdrStatus->_menu.items[vdrStatus->_menu.drawingRow].event;
         channel = vdrStatus->_menu.drawingRow == na ?
            0 : vdrStatus->_menu.items[vdrStatus->_menu.drawingRow].channel;
      }
      else if (strcmp(group, "selectedRowEvent") == 0)
      {
         if (vdrStatus->_menu.currentRow != na)
            event = &vdrStatus->_menu.items[vdrStatus->_menu.currentRow].event;

         channel = vdrStatus->_menu.currentRow == na ?
            0 : vdrStatus->_menu.items[vdrStatus->_menu.currentRow].channel;
      }

      // this items don't need a channel

      *buf = 0;

      if (strcasecmp(var, "Title") == 0)
      {
         if (!event)
         {
            // No EPG Data available
            // RDS is available only for presentTitle

            if ((vdrStatus->_rds.text.empty()) ||
                (strcmp(group, "present") != 0))
            {
               if (!channel)
                  return buf;

               return channel->Name();
            }
            return vdrStatus->_rds.text.c_str();
         }

         return Str::notNull(event->Title());
      }
      else if (strcasecmp(var, "SubTitle") == 0)
      {
         if (!event)
         {  
            // No EPG Data available
            // RDS is available only for presentSubTitle
            
            if (strcmp(group, "present") == 0)
            {
               if (vdrStatus->_rds.title.empty() &&
                   vdrStatus->_rds.artist.empty())
               {
                  strcpy(buf, tr("No EPG data available."));
                  return buf;
               }

               sprintf(buf, " %s : %s\n%s : %s", tr("Title"),
                       vdrStatus->_rds.title.c_str(), tr("Artist"),
                       vdrStatus->_rds.artist.c_str());
            }
            return buf;
         }
         
         return Str::notNull(event->ShortText());
      }
      else if (strcasecmp(var, "Description") == 0)
      {
         if (!event)
            return buf; // No EPG Data available

         return Str::notNull(event->Description());
      }

      if (!channel || !channel->Number())
      {
         tell(2, "Info: Can't find channel for '%s' in '%s' of row %d", 
              group, var, vdrStatus->_menu.drawingRow);

         return 0;
      }

      // this items don't need a event

      if (strcasecmp(var, "ChannelName") == 0)
         return formatString(channel ? channel->Name() : "", fmt, buf, sizeof(buf));
      else if (strcasecmp(var, "ChannelId") == 0)
         return !channel ? "" : strcpy(buf, channel->GetChannelID().ToString());
      else if (strcasecmp(var, "ChannelNumber") == 0)
         return Str::toStr(channel ? channel->Number() : 0);

      // check the event

      if (!event)
      {
         tell(1, "Info: Missing event for '%s', can't lookup variable in '%s'", 
              group, var);

         return 0;
      }

      // following items need a event
      
      // --------------------------
      // get timers lock
      
#if defined (APIVERSNUM) && (APIVERSNUM >= 20301)
      const cTimers* timers;
         {
         LOCK_TIMERS_READ;
         timers = Timers;
         }
#else
      cTimers* timers = &Timers;
#endif
      
      const cTimer* timer = getTimerMatch(timers, event, &timerMatch);
      
      if (strcasecmp(var, "ID") == 0)
         return Str::toStr((int)event->EventID());
      else if (strcasecmp(var, "StartTime") == 0)
         return formatDateTime(event->StartTime(), fmt, buf, sizeof(buf));
      else if (strcasecmp(var, "EndTime") == 0)
         return formatDateTime(event->EndTime(), fmt, buf, sizeof(buf));
      else if (strcasecmp(var, "Duration") == 0)
         return Str::toStr(event->Duration() / 60);
      else if (strcasecmp(var, "HasTimer") == 0)
         return timer && timerMatch == tmFull ? "1" : "0";
      else if (strcasecmp(var, "HasPartialTimer") == 0)
         return timer && timerMatch == tmPartial ? "1" : "0";
      else if (strcasecmp(var, "HasPartialTimerBefore") == 0)
         return timer && timerMatch == tmPartial &&
            timer->StartTime() >= event->StartTime()
            ? "1" : "0";
      else if (strcasecmp(var, "HasPartialTimerAfter") == 0)
         return timer && timerMatch == tmPartial &&
            timer->StartTime() < event->StartTime()
            ? "1" : "0";
      else if (strcasecmp(var, "IsRunning") == 0)
         return event->SeenWithin(30) && event->IsRunning() ? "1" : "0";
      else if (strcasecmp(var, "Elapsed") == 0)
         return Str::toStr(((int)(time(0)-event->StartTime()) / 60));
      else if (strcasecmp(var, "Remaining") == 0)
      {
         if (time(0) < event->StartTime())
            return Str::toStr(event->Duration() / 60);
         else
            return Str::toStr(((int)(event->EndTime()-time(0)) / 60));
      }
      else if (strcasecmp(var, "Progress") == 0)
      {
         if (time(0) > event->StartTime())
            return Str::toStr((int)(100.0 / ((double)event->Duration() / (double)(time(0)-event->StartTime()))));
         else
            return Str::toStr((int)(100.0 / ((double)event->Duration() / (double)(event->StartTime()-time(0)))));
      }
      else if (strcasecmp(var, "IsRecording") == 0)
         return timerMatch == tmFull && timer && timer->Recording() ? "1" : "0";
      else if (strcasecmp(var, "Banner") == 0)
      {
         string mediaPath;
         string posterPath;

         if (getScraperMediaPath(event, 0, mediaPath, posterPath) == success)
            tell(5, "Got banner path '%s'", mediaPath.c_str());

         return mediaPath.c_str();
      }
      else if (strcasecmp(var, "Poster") == 0)
      {
         string mediaPath;
         string posterPath;

         if (getScraperMediaPath(event, 0, mediaPath, posterPath) == success)
            tell(5, "Got poster path '%s'", posterPath.c_str());

         return posterPath.c_str();
      }
   }

   else
   {
      if (strcasecmp(name, "channelGroup") == 0)
         return vdrStatus->_channelGroup.c_str();

      if (strcasecmp(name, "menuTitle") == 0)
         return vdrStatus->_menu.title.c_str();

      if (strcasecmp(name, "colCount") == 0)
         return Str::toStr(vdrStatus->_menu.items[vdrStatus->_menu.drawingRow].tabCount);

      if (strcasecmp(name, "rowCount") == 0)
         return Str::toStr((int)vdrStatus->_menu.items.size());

      if (strcasecmp(name, "visibleRows") == 0)
         return Str::toStr(vdrStatus->_menu.visibleRows);

      if (strcasecmp(name, "currentRow") == 0)
         return Str::toStr(vdrStatus->_menu.currentRow);

      if (strcasecmp(name, "touchMenu") == 0)
         return Str::toStr(vdrStatus->touchMenu);

      if (strcasecmp(name, "themeVersion") == 0)
         return Thms::theTheme->getThemeVersion().c_str();

      if (strcasecmp(name, "syntaxVersion") == 0)
         return Thms::theTheme->getSyntaxVersion().c_str();

      if (strcasecmp(name, "themeName") == 0)
         return Thms::theTheme->getName().c_str();

      if (strcasecmp(name, "vdrVersion") == 0)
         return VDRVERSION;

      if (strcasecmp(name, "mouseX") == 0)
         return Str::toStr(vdrStatus->mouseX);

      if (strcasecmp(name, "mouseY") == 0)
         return Str::toStr(vdrStatus->mouseY);

      if (strcasecmp(name, "mouseKey") == 0)
         return Str::toStr(vdrStatus->mouseKey);

      if (strcasecmp(name, "calibrationInstruction") == 0)
         return vdrStatus->calibration.instruction.c_str();

      if (strcasecmp(name, "calibrationInfo") == 0)
         return vdrStatus->calibration.info.c_str();

      if (strcasecmp(name, "calibrationCursorX") == 0)
         return Str::toStr(vdrStatus->calibration.cursorX);

      if (strcasecmp(name, "calibrationCursorY") == 0)
         return Str::toStr(vdrStatus->calibration.cursorY);

      if (strcasecmp(name, "calibrationTouchedX") == 0)
         return Str::toStr(vdrStatus->calibration.lastX);

      if (strcasecmp(name, "calibrationTouchedY") == 0)
         return Str::toStr(vdrStatus->calibration.lastY);

      if (strcasecmp(name, "calibrationOffsetX") == 0)
         return Str::toStr(vdrStatus->calibration.settings.offsetX);

      if (strcasecmp(name, "calibrationOffsetY") == 0)
         return Str::toStr(vdrStatus->calibration.settings.offsetY);

      if (strcasecmp(name, "calibrationScaleX") == 0)
         return Str::toStr(vdrStatus->calibration.settings.scaleX, 4);

      if (strcasecmp(name, "calibrationScaleY") == 0)
         return Str::toStr(vdrStatus->calibration.settings.scaleY, 4);

      if (strcasecmp(name, "actRecordingCount") == 0)
         return Str::toStr(vdrStatus->_timers.countRunning());

      if (strcasecmp(name, "actRecordingName") == 0)
         return vdrStatus->_timers.firstRunning();

      if (strcasecmp(name, "actTimersRunning") == 0)
         return Str::toStr(vdrStatus->_timers.running());

      if (strcasecmp(name, "actTimersTitle") == 0)
         return vdrStatus->_timers.title();

      if (strcasecmp(name, "actTimersFile") == 0)
         return vdrStatus->_timers.file();

      if (strcasecmp(name, "actTimersStart") == 0)
         return formatDateTime(vdrStatus->_timers.start(), fmt, buf, sizeof(buf));

      if (strcasecmp(name, "actTimersStop") == 0)
         return formatDateTime(vdrStatus->_timers.stop(), fmt, buf, sizeof(buf));

      if (strcasecmp(name, "menuText") == 0)
         return vdrStatus->_menu.text.c_str();

      if (strcasecmp(name, "STR") == 0)
         return !cDevice::ActualDevice() ? "0" : Str::toStr(cDevice::ActualDevice()->SignalStrength());

      if (strcasecmp(name, "SNR") == 0)
         return !cDevice::ActualDevice() ? "0" : Str::toStr(cDevice::ActualDevice()->SignalQuality());

      if (strcasecmp(name, "unseenMailCount") == 0)
         return Str::toStr(vdrStatus->getUnseenMails());

      if (strcasecmp(name, "hasNewMail") == 0)
         return Str::toStr(vdrStatus->hasNewMail());

      if (strcasecmp(name, "channelHasVtx") == 0)
         return Str::toStr(vdrStatus->_presentChannel && vdrStatus->_presentChannel->Tpid());

      if (strcasecmp(name, "channelHasMultilang") == 0)
         return Str::toStr(vdrStatus->_presentChannel && vdrStatus->_presentChannel->Apid(1));
           
      if (strcasecmp(name, "channelHasDD") == 0)
         return Str::toStr(vdrStatus->_presentChannel && vdrStatus->_presentChannel->Dpid(0));
         
      if (strcasecmp(name, "channelIsEncrypted") == 0)
         return Str::toStr(vdrStatus->_presentChannel && vdrStatus->_presentChannel->Ca());
      
      if (strcasecmp(name, "channelIsRadio") == 0)
      {
         int vp = vdrStatus->_presentChannel ? vdrStatus->_presentChannel->Vpid() : na;

         return Str::toStr(vp == 0 || vp == 1 || vp == 0x1fff);
      }

      if (strcasecmp(name, "videoSizeHeight") == 0)
      {
         int width = 0, height = 0;
         double aspect;
         
         if (cDevice::PrimaryDevice())
            cDevice::PrimaryDevice()->GetVideoSize(width, height, aspect);

         return Str::toStr(height);
      }

      if (strcasecmp(name, "videoSizeWidth") == 0)
      {
         int width = 0, height = 0;
         double aspect;

         if (cDevice::PrimaryDevice())
            cDevice::PrimaryDevice()->GetVideoSize(width, height, aspect);

         return Str::toStr(width);
      }

      if (strcasecmp(name, "time") == 0)
      {
         if (strstr(fmt, "%s") || strstr(fmt, "%S") || strstr(fmt, "%T"))
         {
            // refresh every 1 second
            
            if (!_delay)
               scheduleDrawIn(1000);
         }
         else
         {
            // refresh at full minute
            
            scheduleDrawNextFullMinute();
         }
         
         return formatDateTime(time(0), fmt, buf, sizeof(buf));
      }

      if (strcasecmp(name, "x") == 0)
         return Str::toStr(X());
      if (strcasecmp(name, "y") == 0)
         return Str::toStr(Y());
      if (strcasecmp(name, "width") == 0)
         return Str::toStr(Width());
      if (strcasecmp(name, "height") == 0)
         return Str::toStr(Height());
   }

   status = fail;

   return 0;
}

//***************************************************************************
// Music Variable
//***************************************************************************

const char* cDisplayItem::musicVariable(const char* group, const char* var, 
                                        const char* fmt, int& status)
{
   static char buf[1000+TB];

   if (strcasecmp(var, "Track") == 0)
      return formatString(vdrStatus->_music.track(), fmt, buf, sizeof(buf));
   else if (strcasecmp(var, "Artist") == 0)
      return formatString(vdrStatus->_music.artist.c_str(), fmt, buf, sizeof(buf));
   else if (strcasecmp(var, "Album") == 0)
      return formatString(vdrStatus->_music.album.c_str(), fmt, buf, sizeof(buf));
   else if (strcasecmp(var, "Genre") == 0)
      return vdrStatus->_music.genre.c_str();
   else if (strcasecmp(var, "Year") == 0)
      return vdrStatus->_music.year.c_str();
   else if (strcasecmp(var, "Filename") == 0)
      return formatString(vdrStatus->_music.filename.c_str(), fmt, buf, sizeof(buf));
   else if (strcasecmp(var, "Comment") == 0)
      return vdrStatus->_music.comment.c_str();
   else if (strcasecmp(var, "Frequence") == 0)
      return Str::toStr(vdrStatus->_music.frequence/1000);
   else if (strcasecmp(var, "Bitrate") == 0)
      return Str::toStr(vdrStatus->_music.bitrate/1000);
   else if (strcasecmp(var, "StereoMode") == 0)
      return vdrStatus->_music.smode.c_str();
   else if (strcasecmp(var, "Index") == 0)
      return Str::toStr(vdrStatus->_music.index);
   else if (strcasecmp(var, "Count") == 0)
      return Str::toStr(vdrStatus->_music.cnt);
   else if (strcasecmp(var, "CurrentTrack") == 0)
      return vdrStatus->_music.currentTrack.c_str();
   else if (strcasecmp(var, "PlayStatus") == 0)
      return vdrStatus->_music.status.c_str();
   if (strcasecmp(var, "CoverName") == 0)
      return vdrStatus->_coverPath.c_str();
   else if (strcasecmp(var, "Rating") == 0)
      return Str::toStr(vdrStatus->_music.rating);
   else if (strcasecmp(var, "Loop") == 0)
      return Str::toStr(vdrStatus->_music.loop);
   else if (strcasecmp(var, "Timer") == 0)
      return Str::toStr(vdrStatus->_music.timer);
   else if (strcasecmp(var, "Copy") == 0)
      return Str::toStr(vdrStatus->_music.copy);
   else if (strcasecmp(var, "Lyrics") == 0)
      return Str::toStr(vdrStatus->_music.lyrics);
   else if (strcasecmp(var, "Shuffle") == 0)
      return Str::toStr(vdrStatus->_music.shuffle);
   else if (strcasecmp(var, "Shutdown") == 0)
      return Str::toStr(vdrStatus->_music.shutdown);
   else if (strcasecmp(var, "Recording") == 0)
      return Str::toStr(vdrStatus->_music.recording);
   
   else if (strcasecmp(var, "ButtonRed") == 0)
      return vdrStatus->_music.red.c_str();
   else if (strcasecmp(var, "ButtonGreen") == 0)
      return vdrStatus->_music.green.c_str();
   else if (strcasecmp(var, "ButtonYellow") == 0)
      return vdrStatus->_music.yellow.c_str();
   else if (strcasecmp(var, "ButtonBlue") == 0)
      return vdrStatus->_music.blue.c_str();

   status = fail;

   return 0;
}
