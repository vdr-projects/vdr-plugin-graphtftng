/*
 *
 * ----------------------------------------------
 * VDR Plugin 'graph-tft-ng' - Revision History
 * ----------------------------------------------
 *
 */

#define _VERSION      "0.6.16"
#define VERSION_DATE  "13.02.2017"
#define THEMEVERSION  "0.4.1"

#ifdef GIT_REV
#  define VERSION _VERSION "-GIT" GIT_REV
#else
#  define VERSION _VERSION
#endif

/*
 * ------------------------------------

#109 Version 0.6.16, horchi 13.02.2017
     - change: porting to newer libavutil and g++ 6.2

#108 Version 0.6.15, horchi 24.11.2016
     - change: Changed default host of graphtft-fe to localhost

#107 Version 0.6.14, horchi 11.05.2016
     - bugfix: Fixed timer state handling for epg2vdr timer

#106 Version 0.6.13, horchi - 03.05.2016
     - added: Trigger to update timerlist

#105 Version 0.6.12, horchi - 02.05.2016
     - added: support timer list even for remote timers via epg2vdr
              -> enable WITH_EPG2VDR in Makefile

#104 Version 0.6.11, horchi - 02.05.2016
     - added: added test code for epg2vdr timer service interface

#103 Version 0.6.10, horchi - 15.02.2016
     - bugfix: fixed display of setup pages

#102 Version 0.6.9, horchi - 04.02.2016
     - bugfix: fixed typo

#101 Version 0.6.8, horchi - 04.02.2016
     - bugfix: fixed another possible divide by zero

#100 Version 0.6.7, horchi - 04.02.2016
     - bugfix: fixed possible divide by zero

#99 Version 0.6.6, horchi - 03.02.2016
     - bugfix: fixed menu display

#98 Version 0.6.5, horchi - 02.02.2016
     - change: now support max of 10 columns for 'Column' base menues (instead off 6)

#97 Version 0.6.4, horchi - 01.02.2016
     - bugfix: Fixed patch für vdr 2.2.0 and 2.3.1 (no recording data was displayed)
     - bugfix: Fixed detection of menu section (no recording data was displayed)

#96 Version 0.6.3, horchi - 05.01.2016
     - changed: changed header path for libexif

#95 Version 0.6.2, horchi - 23.12.2015
     - added:  autoflip by exif data for jpeg images

#94 Version 0.6.1, horchi - 23.12.2015
     - added: recursive scan of image folder for 'imageDir' item

#93 Version 0.6.0, horchi - 22.12.2015
     - bugfix: Switch of display for xorg usage via svdrpsend now working in all cases
     - added: added new theme item imageDir for dia show, use like:
                 ImageDir x=0,y=0,width=1360,height=768,path=/path-to-jpegs,fit=yes,aspect_ratio=yes,rotate=1,delay=3;
              It don't scans recursive, this will be implemented later

#92 Version 0.5.7, horchi - 14.12.2015
     - added: Switch of display for xorg usage via svdrpsend
     - added: Switch of resolution of xorg display via svdrpsend

#91 Version 0.5.6, horchi - 13.11.2015
     - added: Support of scraper images even for recordings, in theme use:
               (recording|replay|rowRecording|selectedRowRecording)Banner like {presentBanner}
               (recording|replay|rowRecording|selectedRowRecording)Poster like {presentPoster}

#90 Version 0.5.5, horchi - 12.11.2015
     - change: cleared status interface from channel- and recording- locks
               in context of vdr 2.3.1

#89 Version 0.5.4, horchi - 11.11.2015
     - added: Support of scraper images, in theme use:
               (present|followin|event)Banner like {presentBanner}
               (present|followin|event)Poster like {presentPoster}

#88 Version 0.5.3, horchi - 08.11.2015
     - added: Display of channel group with {channelGroup} while zapping

#87 Version 0.5.2, horchi - 08.11.2015
     - bugfix: fixed wrong channel display since vdr 2.3.1 porting

#86 Version 0.5.1, horchi - 07.11.2015
     - bugfix: fixed compile with vdr < 2.3.1
     - added switch to compile with old patch
     - change: A attach on alredy attachtd display force a re-attach now

#85 Version 0.5.0, horchi - 16.10.2015
     - change:   VDR 2.3.1 porting
     - change:   desupport of VDR < 2.2.0
     - change:   redesign of VDR patch, less invasive
                  -> therefore some skin porting may necessary
                     since the naming of the menu section now
                     use the category name (prefixed by 'Menu') defined by vdr

#84 Version 0.4.17, horchi - 03.08.2015
     - added:   autorotate by exif data for jpeg images

#83 Version 0.4.16, horchi - 05.05.2015
     - bugfix:   fixed core of patch from 0.4.15

#82 Version 0.4.15, horchi - 13.04.2015
     - bugfix:   delete old radio data by switching
                 radio channel (patch provided by Ulrich Eckhardt)

#81 Version 0.4.14, horchi - 26.02.2015
     - added:    added check-builddep target (by Lars)

#80 Version 0.4.14, horchi - 02.02.2015
     - bugfix:   fixed window position for FE - reported by nobanzai
     - added:    make install for graphtft-fe provided by 3PO

#79 Version 0.4.13, horchi - 26.10.2014
     - bugfix:   fixed crash direct recording

#78 Version 0.4.12, horchi - 05.10.2014
     - bugfix:   fixed crash on changes events

#77 Version 0.4.11, horchi - 02.03.2014
     - added:   min to theme variables x, y, width and height

#76 Version 0.4.10, horchi - 01.03.2014
     - added:   align_v to partingLine, remove leading -> from partingLine text

#75 Version 0.4.9, horchi - 22.02.2014
     - added:   background image for 'Image' item (path2=)

#74 Version 0.4.8, horchi - 21.02.2014
     - bugfix:  fixed framebuffer output (many thanks to reufer)

#73 Version 0.4.7, horchi - 20.02.2014
     - added:   background image for TextList items (path=)
     - added:   svdrp commands to attach/dettach (ATTA/DETA) X connection (for xorg output)
     - added:   start detached if xorg not available
     - added:   apply xorg resolution without restart
     - bugfix:  fixed volume bar redraw
     - bugfix:  fixed minor redraw bugs
     - bugfix:  fixed scaling problem with xorg output

#72 Version 0.4.6, horchi - 11.12.2013
      - added:  theme variables x, y, width and height
      - added:  Calc feature even to theme attribute width and x (like y of version 0.4.4)

#71 Version 0.4.5, horchi - 10.12.2013
      - bugfix: Fixed possible crash on empty menu items?!
      - added:  Calc feature even to theme attribute height (like y of version 0.4.4)

#70 Version 0.4.4, horchi - 10.12.2013
      - added:  New theme variable (present,following,rowEvent,selectedRowEvent)ChannelId
      - added:  New theme variable lastHeight which is set to
                the dynamic hight of the last drawn TextList item
      - added:  Calc feature to theme attribute y, therefore expressions like
                y=3 + 4 and y=20 + {lastHeight} now supported (space around operator needed).

#69 Version 0.4.3, horchi - 10.11.2013
      - added:  Support of multiple variable files per section
      - added:  Format option lower and upper to theme string variables
      - bugix:  Fixed volumebar (reportet by 3po)
      - bugix:  Fixed marquee and ticker effect
      - added:  new theme variables color and bg_color in format R/G/B/A, for example
                you can now also use "color=220/125/30/255" instead of
                "red=220,green=125,blue=30,transparent,255"
      - added:  new theme variables alpha which can used instead of transparent,
                transparent was the wrong naming as long 255 mean opaque am 0 mean fully transparent
      - added:  multi line text scroll
      - added:  X renderer - direct X support from plugin without graphtft-fe,
                activate with "-d xorg:...."
      - change: Refresh only changed areas - only supported by X renderer (-d xorg:...)

#68 Version 0.4.2, horchi - 03.10.2013
      - added:  option to ignore ESC key
      - change: updated some log messages
      - bugfix: fixed seldom crash at exit

#67 Version 0.4.1, horchi - 16.05.2013
      - change: replaced wildcard (?, ??) feature for image path with range (x-y)
                  due to conficts with ? in path names

#66 Version 0.4.0, horchi - 26.04.2013
      - change: removed DVB and DFB support
      - change: removed PVR350 support
      - change: removed image-magick dependency
      - added:  theme attribute overlay {yes|no} to avoid draw and clean of background
      - change: renamed theme variable actRecordings to actTimers. Please adjust your theme(s)!
      - change: changed default device to 'none' (instead of FB)
      - added:  new theme variables (actTimersTitle,actTimersFile,actTimersStart,actTimersStop,actTimersRunning)
                for TextList item
      - change: removed TextList variables actRunningRecordings and actPendingRecordings
      - added:  '@' sign to define a linfeed in the text parameter
      - change: updated theme version to 0.4.0 due to theme changes
      - change: removed argument path2 for items EventColumn, MenuColumn and Menu
                use path instead
      - bugfix: Update timer list after start
      - added:  new theme variables channelHasVtx, channelHasMultilang,
                channelHasDD, channelIsEncrypted and channelIsRadio
      - change: removed theme items SymVTX, SymDD, SymCrypt and Sym2ch
      - change: added theme variable unseenMailCount (-1 if not avalible), removed theme item MailCount
      - change: added theme variable hasNewMail, removed theme item MailSymbol
      - added:  multi client support
      - added:  set og jpeg quality to TCP protocol
      - added:  theme variabled xxxElapsed and xxxRemaining for present,following,selectedRowEvent,rowEvent,event
      - change: #ifdef now working outside section scope
      - change: fixed missing network/host byte order in tcp communication


----------------------------------------------
----------------------------------------------
Started new branch - graphTFT NG
----------------------------------------------
----------------------------------------------


----------------------------------------------
VDR Plugin 'graph-tft' - Revision History
----------------------------------------------


#65 Version 0.3.12, horchi - 22.04.2013
      - added:  patch for interface to the music plugin, thanks to Dominik (dommi)
      - added:  patch for rec symbol, thanks to Ulrich Eckhardt
      - bugfix  Fixed missing background of some items since 0.3.11, reportet by Ulrich Eckhardt
      - change: use path instead of path2 for EventColumns progress bar images,
                (path2 is supported for compatibility)

#64 Version 0.3.11, horchi - 19.04.2013
      - added:  svdrp command DUMP to trigger dump of current frame
      - bugfix: Fixed double printed partingLine
      - added:  ubuntu-raring patch from Gerald (gda), thx

#63 Version 0.3.10, horchi - 17.04.2013
      - added:  Feature, 'chg_' in image file-name now disable imlib cache (README for details)
      - added:  svdrp command PREV/NEXT to toggle NormalMode views
      - bugfix: Fixed problem storing last selected 'NormalMode' in setup.conf

#62 Version 0.3.9, Ulrich Eckhardt
      - change: Ported to 1.7.35 and new makefile style

#61 Version 0.3.8, horchi
      - change: Ported to VDR 1.7.33

#61 Version 0.3.7, Ulrich Eckhardt
      - change: Ported to VDR 1.7.29
      - change: Ported to actual ffmpeg libs.
      - added:  Patch for vdr 1.7.31

#60 Version 0.3.6, horchi
      - bugfix: added trigger on timer add and del
      - added:  added patch for temporary 'normal' view Mode - thanks to mini73

#59 Version 0.3.5, horchi
      - added:  timers to TextList item.
                New variables (actRecordings, actRunningRecordings, actPendingRecordings)

#58 Version 0.3.4, horchi
      - change: Ported to VDR 1.7.26 (thanks to yavdr team)
      - change: Ported to new image-magick

#57 Version 0.3.3, horchi
      - added:  added patch for plain VDR 1.7.4 (for use without zulu's extension patch)
      - change: better handling to lookup ffmpeg header and libraries, thanks to ronnykornexl
      - change: ported to vdr 1.7.4
      - added:  Update of italien translation, thanks to Diego Pierotto

#56 Version 0.3.2, horchi
      - bugfix: Fixed some memory bugs (thanks to Dominik)
      - added:  First version of KDE plasmoid frontend
      - change: 64 bit porting
      - bugfix: variable lookup problem
      - added:  noadcall.sh example to copy (store) EPG images after recording
      - added:  Support of more than 10 EPG Images by '??' instead of '?' in theme file
      - added:  keyboard support for plasma applet
      - change: Middle click on plasma-applet assigned to 'back' due to right
                click open the popup menu
      - added:  check command to TCP communication to detect and kick crashed clients
      - bugfix: fixed probblem with 'blocking' TCP connections
      - bugfix: fixed crash with directFB output
      - change: improved Makefile to find ffmpeg header (again)

#55 Version 0.3.1, horchi
      - added:  support of mouse whipes (touch whipe planned)
      - added:  svdrp command VIEW without a argument to switch to next normalView
      - added:  Support of key series for on_(dbl)click
      - added:  Support of 'ms' suffix for delay argument in theme
      - bugfix: Fixes missing 'graphtft --help'
      - bugfix: Fixes scale problem for touch calibration
      - bugfix: Missing '[' ']' in menus now displayed correct
      - added:  animated images (up to 10 images displayed in a loop)
      - bugfix: Fixes scale problem for touch calibration for eGalaxy Touchscreens
      - bugfix: Fixed config of snapshot path
      - bugfix: Fixed force redraw
      - bugfix: Minor bugfixes of X-Frontend

#54 Version 0.3.0, horchi
      - change: QT not more needed by the X frontend
      - added:  Cyclic configuable force redraw (via setup.conf)
      - added:  Now Support of linefeeds in item definition (theme file)
      - added:  Theme variable 'foreground' to force items in foreground
      - added:  cascading ifdefs in themes
      - added:  support for touchMenus
      - added:  Global and section theme variables
      - added:  Support of touch devices
      - added:  Group title in setup now displayed on the tft
      - bugfix: Crash on empty menus lists (reported by steffx)
      - bugfix: Fixed display of event**** Variables
      - bugfix: Fixed problem with display of wrong channel during
                recording on primary card (thanks to googles!)
      - change: Optimize Theme-Syntax (f.e. Delete "item=" ),
      - added:  Added if to theme parser
      - bugfix: Many other litle Bugfixes

#53 Version 0.2.2, horchi
      - bugfix: Fixed problem with empty events
      - added:  New format instruction 'tologo' for theme variables
      - change: Renamed svdrp command NORMALVIEW to VIEW (scripts/dia.sh ported)
      - bugfix: Fixed refresh of OSD message
      - change: Improved Makefile
      - added:  Warning if theme version does not match

#52 Version 0.2.1, horchi
      - bugfix: Fixed Missing 'include string.h' (thanks to ronnykornexl)
      - added:  Two new theme expressions (volumeMute and volumeLevel)
      - bugfix: Fixed include problem

#52 Version 0.2.0, horchi
      - added:  redesign of EPG menues (new theme
                items EventColumn and EventColumnSelected)

#51 Version 0.1.23-alpha, horchi
      - added:  font search path variable (fontPath)
                to the theme item
      - bugfix: minor fix in cover handling (if cover not set
                by music plugin)
      - added:  now supporting transparent texts together
                with backround image

#50 Version 0.1.22-alpha, horchi
      - bugfix: Fixed MenuKind of graphTFTMenu

#49 Version 0.1.21-alpha, horchi
      - bugfix: Fixed crashes and scale problems when using DFB output
                (thanks to morone!)
      - added:  Support of image scale to DFB renderer (reported by morone)
      - bugfix: Fixed text output (nothing was displayed id text don't fit
                inconfigured dimensions) DFB renderer (reported by morone)
      - added:  Support for music plugin
      - added:  Display 'condition' for all items (see HOWTO-Themes)

#48 Version 0.1.20-alpha, horchi
      - added:  patch for plain vdr (provided by 'GetItAll')
      - change: time field update cycle now depending on their format string
      - added:  Service interface for music covers

#47 Version 0.1.19-alpha, horchi
      - bugfix: fixed compile error with fbrenderer
      - added:  new condition (colcount = x) for menu configuration
      - bugfix: update time view (fix provided by 'machtnix')

#46 Version 0.1.18-alpha, horchi
      - bugfix: fixed refresh (hide) of volume bar (reported by ckone)
      - bugfix: update total time during replay (reported by ckone)
      - added:  italian translation, thanks to Diego.

#45 Version 0.1.17-alpha, horchi
      - bugfix: added auto-refresh to svdrp command NORMALVIEW
      - change: improved dia script
      - bugfix: fixed order of included items
      - added:  #ifndef for theme file
      - change: improved recording and epg image path
      - buxfix: fixed column width calculation for
                'old style' menues (items MenuSelected and Menu)
      - added:  More DeepBlue like Volume display to demo theme,
                previous style can activated in theme file
                (see #define VOL_STYLE_BLUE)
      - buxfix: fixed un-initialized variable in iconv call

#44 Version 0.1.16-alpha, horchi
      - change: Fixed compile with vdr 1.4.7

#43 Version 0.1.15-alpha, horchi
      - change: Rework of the README
      - change: Re-Implemented the menuMap feature

#42 Version 0.1.14-alpha, horchi
      - change: comment sign now "//"
      - bugfix: fixed crash while playing mp3s
      - added:  #define, #ifdef for theme files
      - bugfix: fixed seldom crash during reload of theme file
      - added:  mail count to mailbox plugin interface
                (ab mailbox plugin version-0.5.2-pre3,
                oder dir 0.5.0 mit dem beiliegenden Patch)

#41 Version 0.1.13-alpha, horchi
      - added: support mailbox plugin (mail symbol)
      - added: scrolling for epg movie and recording description
      - added: image for epg movie and recording description

#40 Version 0.1.12-alpha, horchi
      - added:  support of gettext (ab vdr 1.5.7)
      - change: cleanup of i18n.c
      - change: gcc 4.2 porting
      - change: using 720x576 for dvb renderer
                (setup of width and height are ignored for dvb device)

#39 Version 0.1.11-alpha, horchi
      - bugfix fixed iconv buffer size

#38 Version 0.1.10-alpha, horchi
      - bugfix fixed iconf integration for imlib
      - added display width and height to plugin setup

#37 Version 0.1.9-alpha, horchi
      - changed now using iconv for font conversion

#36 Version 0.1.8-alpha, horchi
      - added SVDRP command 'REFRESH' to force redraw
      - added SVDRP command 'ACTIVE' to activate/deactivate display
      - added Menu entry to activate/deactivate display
      - added SVDRP command 'NORMALVIEW' to change the 'normal view'
      - added aspectRatio and fit for Image and ImageFile items
      - added Dia section to example theme to show how a dia show can setup
      - added expample script dia.sh for dia show

#35 Version 0.1.7-alpha, horchi
      - bugfix crash with DVB renderer
        (thanks to mig for reporting an testing)
      - change renamed some parameter in setup.conf
      - added  support of newer ffmeg versions
        (thanks to Holger Brunn for the patch)

#34 Version 0.1.6-alpha, horchi
      - bugfix crash with unknown reference path of sysindo item
      - bugfix crash on shutdown
      - change implemented missing charWidthOf of DFB renderer
      - added theme with and hight to themefile (for theme scaling)
      - change accelerated scaling of imlib renderer
      - change scaling with imlib renderer now only done if needed
      - change speed up copy to framebuffer in 16 bit colordepth

#33 Version 0.1.5-alpha, horchi
      - bugfix added refresh for SpectrumAnalyzer Item
      - change updated ReplayMP3 section of demo theme
      - added cover patch for mp3 plugin version mp3-0.9.15pre14
      - bugfix fixed missing update of replay sections

#32 Version 0.1.4-alpha, horchi
      - bugfix crash on missing theme sections
      - change autodetect libgtop path in makefile
      - change removed 'picture beside picture' menu entry
      - added auto fill of 'NormalView' menu depending on theme file
      - added compatibility with older libgtop versions (pre 2.14.8)

#31 Version 0.1.3-alpha, horchi
      - bugfix problem with item 'datetimeformat' fixed
      - change now string is used to identify menu sections (patch changed!)
      - added  Other section names in epgsearch- and extrec- patch,
               due to this the standard vdr sections and the sections
               for the plugins can coexist in the theme file

#30 Version 0.1.2-alpha, horchi
      - bugfix core on channel change in program info of epgsearch
      - added  support for inline comments in theme files

#29 Version 0.1.1-alpha, horchi
      - bugfix permanent redraw at some systems due to uninitialized variable

#28 Version 0.1.0-alpha, horchi
      - change removed compatibility properties 'transperents'/'bg_transperents',
               use 'transparent'/'bg_transparent' instead
      - bugfix static image of menuColumnSelected items was not displayed
      - change replaced item 'Date' with 'DateTimeFormat'
      - change removed properties 'pathBACK' and 'pathFRONT'
      - change ShowMutePermanent moved fom setup to theme (permanent={yes|no})
      - change renamed theme property bg_heigth in bg_height
      - change renamed theme item ReplayProcessbar in ReplayProgressbar
      - added  'Defaults' item to setup property defaults of
               all items in the current section
      - change removed Item VolumeBackground, for background image
               now path2 of Volumebar is used
      - added  force_refresh property

#27 Version 0.0.20, horchi
      - added scroll support for Colum-Items (see HOWTO.Themes also)
      - fixed some problems in scroll mode

#26 Version 0.0.19, horchi
      - added OSD flip patch from Markus (mahlzeit)

#25 Version 0.0.18, horchi
      - added VDR 1.5.x support

#24 Version 0.0.17, horchi
      - added keyboard service for x frontend

#23 Version 0.0.16, horchi
      - added DateTimeFormat Item (siehe HOWTO.Themes)

#22 Version 0.0.15a, horchi
      - added flag and button to touchTFT interface

#21 Version 0.0.15, horchi
      - added touchTFT interface (the first step ;)
      - added clock and logo view

#20 Version 0.0.14b, horchi
      - fixed scrolling in case scrollaround is enabled in the OSD setup
      - fixed imageMap handling

#19 Version 0.0.14a, horchi
      - fixed now skipping unknown sections instead of core ;)
      - added patch for extrecmenu
      - added faster im memory image-convert for TCP communication
      - added configuration option for the jpeg quality (0-100) used for
              communication with the x frontend

#18 Version 0.0.14, horchi
      - added mouse support for x frontend
      - fixed mutex lock while reloading themes
      - fixed hide of volume bar!
      - fixed display correct theme after reload
      - fixed display of images/logos without height and/or width

#17 Version 0.0.13, horchi
     - fixed jumpy menu scrolling
       (calc of the displayed top line totally reworked)
     - added auto-scale for the column images
     - fixed parser bug (e.g. static_x= is found for x=)
     - added image with static position for menu cloumns
     - added the image_map feature (known from 'normal' menues)
       to the colunm oriented menus
     - added bool handling to the theme parser
             for the boolean parameters (like stat_pic, image_map, switch, animated)
             now yes/no, true/false and 1/0 can used
     - added new spacing attribute to themes parser for column-items
     - hight of progress bar nor configurable (in percent of the row height)
     - bugfix: DumpImageX, DumpImageY, UseStillPicture not loaded from setup.conf
     - added include to theme config so sections can include others
     - themes now don't loaded twice at startup
     - image support for progress bars
     - added reload thems via Main-Menu, if it was enabled in Makefile
       (helpful during creating/testing themes)
     - fixed theme change in setup (new bug since 0.0.12)
     - graphical (image) progress now also for item timeline

#16 Version 0.0.12a, horchi
     - added column type 'image' e.g. for the status column (T,t,V,...)
     - fixed problem with imagemap's using setup plugin
       (now images for Menu's ending with "...")
     - added span support (first BETA to be continued!)

#15 Version 0.0.12, horchi
     - added own thread for the tcp communication

#14 Version 0.0.11b, horchi
     - New sections WhatsOnNow, WatchsOnNext and WhatsOnElse for epgsearch
       WhatsOnElse is the smae than WhatsOn (only a epgseach compatible name)
     - New section MenuSetupPage

#13 Version 0.0.11, horchi
     - fixed typo 'transperents' in theme, transitional both spellings
       can be used.

#12 Version 0.0.10b, horchi
     - added parting line handling for epgsearch
     - added tcp communication for X frontend

#11 Version 0.0.10a, horchi
     - minor changes

#10 Version 0.0.10,  horchi - 23.10.2006
     - added menu column support
       so you can define each column of a menu as you need
       (as example see enhanced DeepBlue Skin)

     - more menu sections are available, these are:
       MenuMain
       MenuSchedule
       MenuChannels
       MenuTimers
       MenuRecordings
       MenuSetup
       MenuCommands
       MenuWhatsOn
       MenuWhatsOnNow
       due to this feature you can give (nearly) each menue it's own look,
       for the rest and the sections from above wich dosen't apper n the Skin
       as default the 'Menu' Section will be used.
       For all menues the 'traditional' and likewise the column oriented configuration
       method is available.

     - graphical progress bar for MenuWhatsOnNow
     - fixes timing bug of dump image (image never dumped since cTimeMs is used)
     - included patch (found at vdr-portal) to support ffmeg newer than 0.4.8
     - added support of APIVERSION to the Makefile
       (and removed support of really old VDR Versions from the code)

Version 0.0.5 - 0.0.9
     - i don't know ;)

Version 0.0.4
     - added Menu-Icons
     - added Tabs support in Menus
     - added new Menu Item
     - removed PbP-Mode (will be reworked soon)

Version 0.0.3-pre5

Version 0.0.3-pre4

Version 0.0.3-pre3

Version 0.0.3-pre2
     - fixed a version dependency in transfer.c
     - fixed missing include file on some systems in display.c
     - fixed display of currently recording timers (stopped timers were not removed)
     - fixed possible race condition in update thread
     - fixed update in replay mode (now exactly once a second)
     - fixed display of group names and pending channel switches
     - fixed OsdClear() behaviour for status message without menu
     - fixed update after stopping a replay
     - moved thx to readme
     - fix in volumebackgorund
     - added following event title/subtitle to UpdateProgramme()
     - fixed possible NULL pointer handling and update condition in OsdProgramme()
     - added device deinitialization
     - inserted slight delay on update thread startup (improves vdr startup)
     - added animation code
     - fixed display of OsdChannel for some OSD themes (Elchi for example)
     - removed stale include file menu.h from graphtft.c

Version 0.0.3-pre1
     - most code rewritten and reordered

Version 0.0.2
     - add PbP mode
     - add Theme support

Version 0.0.1
     - Initial revision.

 * ------------------------------------
 */
