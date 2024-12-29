//***************************************************************************
// Group VDR/GraphTFT
// File service.h
// Date 21.11.06
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
// (c) 2006-2008 Jörg Wendel
//--------------------------------------------------------------------------
// Class cGraphTftService
//***************************************************************************

#ifndef __GTFT_SERVICE_H__
#define __GTFT_SERVICE_H__

#define GRAPHTFT_TOUCH_EVENT_ID "GraphTftTouchEvent-v1.0"
#define GRAPHTFT_CALIBRATION_ID "GraphTftCalibration-v1.0"

#define GRAPHTFT_COVERNAME_ID   "GraphTftCovername-v1.0"
#define GRAPHTFT_PLAYLIST_ID    "GraphTftPlaylist-v1.0"
#define GRAPHTFT_STATUS_ID      "GraphTftStatus-v1.0"
#define GRAPHTFT_HELPBUTTONS_ID "GraphTftHelpButtons-v1.0"
#define GRAPHTFT_INFO_ID        "GraphTftInfo-v1.0"

//***************************************************************************
// cGraphTftService
//***************************************************************************

class cGraphTftComService
{
   public:

      // Touch TFT Interface

      enum Command
      {
         cmdUnknown = na,

         cmdWelcome,
         cmdData,
         cmdMouseEvent,
         cmdLogout,
         cmdStartCalibration,
         cmdStopCalibration,
         cmdCheck,
         cmdJpegQuality
      };

      enum MouseButton
      {
         mbLeft   = 1,
         mbMiddle,
         mbRight,
         mbWheelUp,
         mbWheelDown
      };

      enum EventFlags
      {
         efNone        = 0x00,

         efDoubleClick = 0x01,
         efShift       = 0x02,
         efAlt         = 0x04,
         efStrg        = 0x08,
         efKeyboard    = 0x10,
         efHWhipe      = 0x20,
         efVWhipe      = 0x40
      };

#pragma pack(1)
      struct GraphTftTouchEvent
      {
         int x;
         int y;
         int button;
         int flag;
         int data;
      };
#pragma pack()

      struct GraphTftCalibration
      {
         int activate;
      };

      // Music Plugins

      struct MusicServiceCovername
      {
         const char* name;
      };

      struct MusicServicePlaylist
      {
         int index;
         int count;
         const char* item;
      };

      struct MusicServicePlayerInfo
      {
         const char* filename;
         const char* artist;
         const char* album;
         const char* genre;
         const char* comment;
         int year;
         double frequence;
         int bitrate;
         const char* smode;
         int index;           // current index in tracklist
         int count;           // total items in tracklist
         const char* status;  // player status  
         const char* currentTrack;
         bool loop;
         bool shuffle;
         bool shutdown;
         bool recording;
         int rating;   
      };

      struct MusicServiceHelpButtons
      {
         const char* red;
         const char* green;
         const char* yellow;
         const char* blue;
      };

      struct MusicServiceInfo
      {
         const char* info;
      };
};

typedef cGraphTftComService cTftCS;

//***************************************************************************
#endif //  __GTFT_SERVICE_H__
