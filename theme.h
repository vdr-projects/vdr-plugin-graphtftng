/**
 *  GraphTFT plugin for the Video Disk Recorder 
 * 
 *  theme.h - A plugin for the Video Disk Recorder
 *
 *  (c) 2004 Lars Tegeler, Sascha Volkenandt
 *  (c) 2006-2013 J�rg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 **/

#ifndef __GTFT_THEME_H
#define __GTFT_THEME_H

#include <errno.h>

#include <string>
#include <map>
#include <vector>
#include <stack>

#include <common.h>
#include <renderer.h>
#include <string>

#include <vdr/tools.h>
#include <vdr/config.h>

#define SECONDS(x) (((uint64_t)x)*1000)

using std::string;
using std::map;

class cDisplayItem;
class cGraphTFTDisplay;
class cThemeSection;

//***************************************************************************
// Variable Provider
//***************************************************************************

class VariableProvider
{
   public:
      
      VariableProvider() {};
      virtual ~VariableProvider() {};

      virtual string channelLogoPath(const char* channel, 
                                     const char* format = 0, int classic = yes);

      virtual int variableOf(string& name, const char* expression, char*& e);
      virtual int evaluate(string& buf, const char* var);
      virtual const char* splitFormatValue(const char* data, 
                                   char* value, char* format);

      virtual int lookupVariable(const char* name, string& value,
                                 const char* fmt = 0) = 0;

      int calcExpression(const char* expression);
      int calc(const char* op, int left, int right);
};

//***************************************************************************
// Theme Service
//***************************************************************************

class cThemeService
{
   public:

      enum ReplayMode
      {
         rmUnknown = na,
         rmPlay,
         rmForward,
         rmSpeed
      };

      enum ItemUnit
      {
         iuUnknown = na,
         iuAbsolute,
         iuRelative,
         iuPercent
      };

      enum ConditionArgumentType
      {
         catUnknown,
         catInteger,
         catString
      };

      enum MenuItemType
      {
         itNormal,
         itPartingLine
      };

      // groups

      enum eThmeGroup
      {
         groupUnknown   = 1,

         groupChannel   = 2,
         groupVolume    = 4,
         groupButton    = 8,
         groupRecording = 16,
         groupReplay    = 32,
         groupMessage   = 64,
         groupMenu      = 128,
         groupMusic     = 256,
         groupTextList  = 512,
         groupCalibrate = 1024,
         groupVarFile   = 2048,

         groupAll = 0xFFFF
      };

      // items

      enum ItemKind
      {
         itemUnknown = na,

         itemSectionInclude,                // = 0
         itemTheme,

         itemBegin,                         // = 2
         itemText = itemBegin,              // = 2
         itemImage,                         // = 3
         itemImageFile,
         itemImageDir,
         itemRectangle,                     // = 5
         itemTimebar,

         itemMessage,
         itemVolumeMuteSymbol,
         itemVolumebar,

         itemMenu,
         itemMenuSelected,

         itemMenuButton,
         itemMenuButtonRed = itemMenuButton, // = 
         itemMenuButtonGreen,
         itemMenuButtonYellow,
         itemMenuButtonBlue,

         itemMenuButtonBackground,           // = 
         itemMenuButtonBackgroundRed = itemMenuButtonBackground,  // = 
         itemMenuButtonBackgroundGreen,
         itemMenuButtonBackgroundYellow,
         itemMenuButtonBackgroundBlue,

         itemMenuImageMap,                   // = 

         itemSpectrumAnalyzer,
         itemPartingLine,                    // = 
         itemSysinfo,
         itemBackground,
         itemTextList,
         itemProgressbar,

         itemClickArea,
         itemMenuNavigationArea,
         itemCalibrationCursor,

         itemColumn,
         itemColumnSelected,

         itemEventColumn,
         itemEventColumnSelected,

         itemDefaults,
         itemVarFile,

         itemCount
      };

      enum Misc
      {
         maxPathCount = 10
      };

      enum Align
      {
         algLeft,
         algCenter, 
         algRight
      };

      enum ScrollModes
      {
         smOff,
         smMarquee,
         smTicker
      };

      struct Translation
      {
         int denum;
         const char* name;
      };

      static int toDenum(Translation t[], const char* value);
      static Translation alignments[];
      static Translation scrollmodes[];

      static const char* toName(ItemKind aItem);
      static ItemKind toItem(const char* aName);
      static int isValid(ItemKind aItem)
         { return aItem > itemUnknown && aItem < itemCount; }

      static const char* items[itemCount+1];
};

typedef cThemeService Ts;

//***************************************************************************
// Class cThemeItem
//  - represents a theme item
//***************************************************************************

class cThemeItem : public cListObject, public cThemeService, public VariableProvider
{  
   public:
      cThemeItem& operator= (const cThemeItem &themeItem) {
         currentSection = themeItem.currentSection;
         lineBuffer = themeItem.lineBuffer;
         condition = themeItem.condition;
         _item = themeItem._item;
         _id = themeItem._id;
         _debug = themeItem._debug;
         _sectionInclude = themeItem._sectionInclude;
         _area = themeItem._area;
         _x = themeItem._x;
         _y = themeItem._y;
         _width = themeItem._width;
         _height = themeItem._height;
         _start_line = themeItem._start_line;
         _overlay = themeItem._overlay;
         _lines = themeItem._lines;
         _line = themeItem._line;
         _size = themeItem._size;
         _switch = themeItem._switch;
         _menu_x = themeItem._menu_x;
         _menu_y = themeItem._menu_y;
         _menu_width = themeItem._menu_width;
         _menu_height = themeItem._menu_height;
         _bg_x = themeItem._bg_x;
         _bg_y = themeItem._bg_y;
         _bg_width = themeItem._bg_width;
         _bg_height = themeItem._bg_height;
         _stat_pic = themeItem._stat_pic;
         _stat_x = themeItem._stat_x;
         _stat_y = themeItem._stat_y;
         _image_map = themeItem._image_map;
         _stat_pic = themeItem._stat_pic;
         _stat_x = themeItem._stat_x;
         _stat_y = themeItem._stat_y;
         _image_map = themeItem._image_map;
         _stat_text = themeItem._stat_text;
         _stat_width = themeItem._stat_width;
         _stat_height = themeItem._stat_height;
         _align = themeItem._align;
         _align_v = themeItem._align_v;
         _count = themeItem._count;
         _number = themeItem._number;
         _index = themeItem._index;
         _spacing = themeItem._spacing;
         _yspacing = themeItem._yspacing;
         _bar_height = themeItem._bar_height;
         _bar_height_unit = themeItem._bar_height_unit;
         _scroll = themeItem._scroll;
         _scroll_count = themeItem._scroll_count;
         _dots = themeItem._dots;
         _permanent = themeItem._permanent;
         _factor = themeItem._factor;
         _aspect_ratio = themeItem._aspect_ratio;
         _fit = themeItem._fit;
         _foreground = themeItem._foreground;
         _rotate = themeItem._rotate;
         _whipe_res = themeItem._whipe_res;
         _delay = themeItem._delay;                      // delay in ms
         _color = themeItem._color;
         _bg_color = themeItem._bg_color;
         _value = themeItem._value;
         _total = themeItem._total;
         _unit= themeItem._unit;
         _reference= themeItem._reference;
         _onClick= themeItem._onClick;
         _onDblClick= themeItem._onDblClick;
         _onUp= themeItem._onUp;
         _onDown= themeItem._onDown;
         _font= themeItem._font;
         _path= themeItem._path;
         _focus = themeItem._focus;
         _path2 = themeItem._path2;
         _type = themeItem._type;
         _format = themeItem._format;
         _text = themeItem._text;
         _condition = themeItem._condition;

         pathCount = themeItem.pathCount;
         for (int i=0; i < maxPathCount; i++) {
            pathList[i] = themeItem.pathList[i];
         }
         section = themeItem.section;
         return *this;
      }
      struct cPath
      {
         string configured;  // configured path

         // path can contain a range [0-9], [3-99], ...
         //   used to support images like thumbnail_1.jpg
         // in this case the following members are set ...

         int curNum;
         string last;        // last path
      };

      cThemeItem();
      virtual ~cThemeItem();
      
      bool Parse(const char *s);
      int parseDirectives(string& toParse);
      int parseVariable(string& toParse, cThemeSection* section = 0);
      virtual int init() { return done; }

      //

      int lookupVariable(const char* name, string& value, const char* fmt = 0);
      int setVariable(const char* name, int value);

      // statics

      static cDisplayItem* newDisplayItem(int item);

      // temporary buffer used by the parser ::Parse()

      static cThemeSection* currentSection;
      static string lineBuffer;
      static string condition;

   protected:

      // functions

      int ParseText(string toParse);
      int ParseVar(string toParse, string name, int* value);
      int ParseVarTime(string toParse, string name, uint64_t* value);
      int ParseVar(string toParse, string name, string* value);
      int ParseVarExt(string toParse, string name, string* value);
      int ParseVarExt(string toParse, string name, int* value);
      int ParseDirective(string toParse, string name, string* value);
      int ParseVar(string toParse, string name, int* value, Translation t[]);
      int ParseVarColor(string toParse, string name, string* value);

      // item properties

      int _item;

      string _id;
      string _debug;
      string _sectionInclude;
      string _area;

      string _x;
      string _y;
      string _width;
      string _height;
      string _start_line;

      int _overlay, _lines, _line, _size, _switch;
      int _menu_x,  _menu_y, _menu_width, _menu_height;
      int _bg_x, _bg_y, _bg_width, _bg_height;
      int _stat_pic, _stat_x, _stat_y, _image_map;
      int _stat_text, _stat_width, _stat_height;
      int _align, _align_v, _count;
      int _number;
      int _index, _spacing, _yspacing, _bar_height, _bar_height_unit;
      int _scroll, _scroll_count, _dots;
      int _permanent, _factor, _aspect_ratio, _fit, _foreground;
      int _rotate;
      int _whipe_res;
      uint64_t _delay;                      // delay in ms

      string _color;
      string _bg_color;
      string _value;
      string _total;
      string _unit;
      string _reference;
      string _onClick;
      string _onDblClick;
      string _onUp;
      string _onDown;
      string _font;
      string _path;
      string _focus;
      string _path2;
      string _type;
      string _format;
      string _text;
      string _condition;

      int pathCount;
      cPath pathList[maxPathCount];
      cThemeSection* section;               // my section
};

//***************************************************************************
// Display Items
//***************************************************************************

class cDisplayItem : public cThemeItem
{
   public:

      cDisplayItem();
      virtual ~cDisplayItem();

      // functions

      virtual const char* nameOf()  { return toName((ItemKind)_item); }
      virtual int groupOf()         { return groupUnknown; }
      virtual int isOfGroup(int g)  { return groupOf() & g; }

      int useVarOf(const char* varPart) 
      { return strstr(_text.c_str(), varPart) || strstr(_path.c_str(), varPart) || strstr(_condition.c_str(), varPart); }

      virtual int draw();
      virtual int refresh();
      virtual int reset();

      // virtual string channelLogoPath(const char* channel, int classic = yes);

      virtual int drawText(const char* text, int y = 0, 
                           int height = na, int clear = yes, 
                           int skipLines = 0);
      virtual int drawRectangle();
      virtual int drawBackRect(int y = 0, int height = 0);
      virtual int drawImage(const char* path = 0, int fit = na, 
                            int aspectRatio = na, int noBack = no);
      virtual int drawImageOnBack(const char* path, int fit = no, int height = na);
      virtual int drawProgressBar(double current, double total, 
                                  string path, int y = na, 
                                  int height = na, 
                                  int withFrame = yes, int clear = yes);
      virtual int drawPartingLine(string text, int y, int height);
     
      virtual void setBackgroundItem(cDisplayItem* p) { backgroundItem = p; }
      virtual void setNextDraw()                      { nextDraw = msNow(); }
      
      virtual uint64_t getNextDraw()                  { return nextDraw; }
      virtual int isForegroundItem()                  { return no; }

      void scheduleDrawAt(uint64_t aTime);
      void scheduleDrawIn(int aTime);
      void scheduleDrawNextFullMinute();

      void setSection(cThemeSection* s)               { section = s; } 

      // statics

      static void scheduleForce(uint64_t aTime);

      static void clearSelectedItem()                        { selectedItem = 0; } 
      static void setForce(int flag)                         { forceDraw = flag; }
      static int getForce()                                  { return forceDraw; }
      static void setRenderer(Renderer* aRender)             { render = aRender; }
      static void setVdrStatus(cGraphTFTDisplay* aVdrStatus) { vdrStatus = aVdrStatus; }

      // Item getter/setter

      int Item()                         { return _item; }
      string Id()                        { return _id; }
      string Area()                      { return _area; }

      int Index(int value = na)          { if (value != na) _index = value; return _index; }
      int Changed()                      { return changed; }

      void setStartLine(int value)       { _start_line = Str::toStr(value); }
      void setX(int value)               { _x = Str::toStr(value); }
      void setY(int value)               { _y = Str::toStr(value); }
      void setWidth(int value)           { _width = Str::toStr(value); }

      int Number(int value = na)         { if (value != na) _number = value; return _number; }

      int StartLine()                    { return optionVariable(_start_line.c_str()); }
      int X()                            { return optionVariable(_x.c_str()); }
      int Y()                            { return optionVariable(_y.c_str()); }
      int Width()                        { return optionVariable(_width.c_str()); }
      int Height()                       { return optionVariable(_height.c_str()); }

      // item options

      int BgX()               { return _bg_x; }
      int BgY()               { return _bg_y; }
      int BgWidth()           { return _bg_width; }
      int BgHeight()          { return _bg_height; }

      int ImageMap()          { return _image_map; }
      int StaticPicture()     { return _stat_pic; }
      int StaticText()        { return _stat_text; }
      int StaticWidth()       { return _stat_width; }
      int StaticHeight()      { return _stat_height; }
      int StaticX()           { return _stat_x; }
      int StaticY()           { return _stat_y; }
      int Overlay()           { return _overlay; }
      int Lines()             { return _lines; }
      int Line()              { return _line; }
      int Size()              { return _size; }  
      int Switch()            { return _switch; }
      int Align()             { return _align; }
      int AlignV()            { return _align_v; }
      uint64_t Delay()        { return _delay; }
      int Foreground()        { return _foreground; }
      int Count()             { return _count; }
      int Spacing()           { return _spacing; }
      int YSpacing()          { return _yspacing; }
      int BarHeight()         { return _bar_height; }
      int BarHeightUnit()     { return _bar_height_unit; }
      int Fit()               { return _fit; }
      int AspectRatio()       { return _aspect_ratio; }
      int Scroll()            { return _scroll; }
      int MenuX()             { return _menu_x; }
      int MenuY()             { return _menu_y; }
      int MenuWidth()         { return _menu_width; }
      int MenuHeight()        { return _menu_height; }
      int WhipeRes()          { return _whipe_res; }

      string Color()          { return _color; }
      string BgColor()        { return _bg_color; }
      string Value()          { return _value; }
      string Total()          { return _total; }
      string Font()           { return _font; }
      string Focus()          { return _focus; }
      string Path()           { return _path; }
      string Path2()          { return _path2; }
      string Type()           { return _type; }
      string Format()         { return _format; }
      string Text()           { return _text; }
      string OnClick()        { return _onClick; }
      string OnDblClick()     { return _onDblClick; }
      string OnUp()           { return _onUp; }
      string OnDown()         { return _onDown; }
      string Condition()      { return _condition; } 
      string SectionInclude() { return _sectionInclude; }
      string Debug()          { return _debug; }

      // 

      int optionVariable(const char* expression)
      { 
         string p;
         string name;
         int min = -32000;
         char* buffer = strdup(expression);
         char* v;

         // evaluate variable
         
         if ((v = strchr(buffer, '/')))
         {
            *v = 0;
            v++;
            min = atoi(buffer);
         }
         else 
            v = buffer;
            
         if (evaluate(p, v) == success)
         {
            int res = calcExpression(p.c_str());

            if (logLevel > 2 && strchr(v, '{'))
               tell(3, "evaluated '%s' to '%s', res is (%d)", v, p.c_str(), res);
         
            free(buffer);

            return std::max(min, res);
         }

         free(buffer);

         return 0;
      }

      int evaluateCondition(int recurse = no);
      p_rgba evaluateColor(const char* var, p_rgba rgba);
      string evaluatePath();
      int lookupVariable(const char* name, string& value, const char* fmt = 0);
      int lineCount()                { return actLineCount; }

   protected:

      const char* variable(const char* name, const char* fmt, int& status);
      const char* musicVariable(const char* group, const char* var, const char* fmt, int& status);

      const char* formatDateTime(time_t time, const char* fmt, char* date, int len, int absolut = no);
      const char* formatString(const char* str, const char* fmt, char* buffer, int len);
      int replayModeValue(ReplayMode rm);
      int condition(int left, int right, const char* op);
      int condition(string* left, string* right, const char* op);

      int haveBackgroundItem() { return backgroundItem && backgroundItem->Path() != ""; }

      // data

      int changed;
      uint64_t nextDraw;
      cDisplayItem* backgroundItem;
      int visible;

      // scroll mode

      int marquee_active;
      int marquee_left;
      int marquee_idx;
      int marquee_count;
      int marquee_strip;
      uint64_t nextAnimationAt;
      int actLineCount;
      int lastConditionState;

      int lastX;
      int lastY;
      int lastWidth;
      int lastHeight;

      // statics

      static int forceDraw;
      static Renderer* render;
      static cGraphTFTDisplay* vdrStatus;
      static cDisplayItem* selectedItem;
      static uint64_t nextForce;
};

//***************************************************************************
// Variable File
//***************************************************************************

class cVariableFile : public cDisplayItem
{
   public:

      cVariableFile() : cDisplayItem() { lastUpdate = 0; _delay = 60 * 1000; }

      int hasName(const char* name) { return _path == name; }
      const char* getName() { return _path.c_str(); }
      
      int lookupVar(string var, string& value)
      { 
         string::size_type p;
         string name, ns;

         value = "";

         if ((p = var.find(".")) == string::npos)
            return fail;

         ns = var.substr(0, p);
         name = var.substr(p+1);

         tell(5, "lookup file variable '%s.%s' in file '%s'", 
              ns.c_str(), name.c_str(), _path.c_str());

         if (_path != ns)
            return fail;

         map<string,string>::iterator iter = variables.find(name);

         if (iter != variables.end())
         {
            value = iter->second.c_str();
            return success;
         }

         return fail;
      }

      int parse()
      {
         int status = ignore;
         FILE* fp;
         char line[500+TB]; *line = 0;
         char *c, *p, *name;
         char* value;

         if (time(0) < lastUpdate + (time_t)_delay/1000)
            return ignore;
 
         variables.clear();

         if (!(fp = fopen(_path2.c_str(), "r")))
         {
            tell(0, "Can't open '%s', error was '%s'",  _path2.c_str(), strerror(errno));
            return ignore;
         }

         while ((c = fgets(line, 500, fp)))
         {
            line[strlen(line)] = 0;        // cut linefeed
            
            if ((p = strstr(line, "//")))  // cut comments
               *p = 0;
            
            if ((p = strstr(line, ";")))   // cut line end
               *p = 0;
            
            // skip empty lines
            
            Str::allTrim(line);
            
            if (Str::isEmpty(line))
               continue;
            
            // check line, search value
            
            if (!(name = strstr(line, "var ")) || !(value = strchr(line, '=')) || name >= value)
            {
               tell(0, "Info: Ignoring invalid line [%s] in '%s'", line, _path2.c_str());
               continue;
            }
            
            name += strlen("var ");
            *value = 0;
            value++;
            
            Str::allTrim(name);
            Str::allTrim(value);
            
            tell(4, "append variable '%s' with value '%s'", name, value);
            variables[name] = value;
            status = success;
         }
         
         fclose(fp);
         lastUpdate = time(0);
         
         return status;
      }

   protected:

      map<string,string> variables;
      time_t lastUpdate;
};

//***************************************************************************
// Display Items
//***************************************************************************

class cDisplayText : public cDisplayItem
{
   public:

      cDisplayText() { lastText = ""; }

      int groupOf()  
      { 
         int group = groupUnknown;

         if (useVarOf("{replay"))
            group |= groupReplay;
         if (useVarOf("{recording"))
            group |= groupRecording;
         if (useVarOf("{actRecording"))
            group |= groupRecording;
         if (useVarOf("{event"))
            group |= groupChannel;
         if (useVarOf("{present"))
            group |= groupChannel;
         if (useVarOf("{following"))
            group |= groupChannel;
         if (useVarOf("{videoSize"))
            group |= groupChannel;
         if (useVarOf("{channel"))
            group |= groupChannel;
         if (useVarOf("{volume"))
            group |= groupVolume;
         if (useVarOf("{calibration"))
            group |= groupCalibrate;

         group |= groupVarFile;

         return group;
      }

      int draw();

   protected:

      string lastText;
};

class cDisplayImage : public cDisplayItem
{
   public:

      int groupOf()
      { 
         int group = groupUnknown;

         if (useVarOf("{replay"))
            group |= groupReplay;
         if (useVarOf("{recording"))
            group |= groupRecording;
         if (useVarOf("{actRecording"))
            group |= groupRecording;
         if (useVarOf("{event"))
            group |= groupChannel;
         if (useVarOf("{present"))
            group |= groupChannel;
         if (useVarOf("{following"))
            group |= groupChannel;
         if (useVarOf("{videoSize"))
            group |= groupChannel;
         if (useVarOf("{channel"))
            group |= groupChannel;
         if (useVarOf("{volume"))
            group |= groupVolume;
         if (useVarOf("{calibration"))
            group |= groupCalibrate;

         return group;
      }

      int draw();
};

class cDisplayTextList : public cDisplayItem
{
   public:

      int groupOf()  
      { 
         int group = groupTextList;

         if (useVarOf("{music"))
            group |= groupMusic;
         if (useVarOf("{actTimers"))
            group |=  groupRecording;

         return group;
      }

      int draw();
};

class cDisplayRectangle : public cDisplayItem
{
   public:

      int draw();
};


class cDisplayBackground : public cDisplayImage
{
   public:

      int groupOf()         { return groupUnknown; }
      int draw();
};

class cDisplayImageFile : public cDisplayItem
{
   public:

      int groupOf()  { return groupReplay; }
      int draw();
};

class cDisplayImageDir : public cDisplayItem
{
   public:

      // int groupOf()  { return groupReplay; }
      int init();
      int draw();

   protected:

      struct ImageFile
      {
         int initialized;
         std::string path;
         unsigned int width;
         unsigned int height;
         unsigned int orientation;
         unsigned int landscape;
      };

      int getNext(std::vector<ImageFile>::iterator& it, ImageFile*& file);
      int scanDir(const char* path, int level = 0);

      std::vector<ImageFile> images;
      std::vector<ImageFile>::iterator current;
};

class cDisplayCalibrationCursor : public cDisplayItem
{
   public:

      int groupOf()  { return groupCalibrate; }
      int draw();
};

class cDisplayMenuButton : public cDisplayItem
{
   public:

      int groupOf()  { return groupButton; }
      int draw();
};

class cDisplayMenuButtonBackground : public cDisplayItem
{
   public:

      int groupOf()  { return groupButton; }
      int draw();
};

class cDisplayMessage : public cDisplayItem
{
   public:

      cDisplayMessage() : cDisplayItem() { visible = no; }
      int groupOf()           { return groupMessage; }
      int isForegroundItem()  { return yes; }

      int draw();
};

class cDisplayVolumeMuteSymbol : public cDisplayItem
{
   public:

      cDisplayVolumeMuteSymbol() : cDisplayItem() { visible = no; } 
      int groupOf()           { return groupVolume; }
      int isForegroundItem()  { return yes; }

      int draw();
};

class cDisplayVolumebar : public cDisplayItem
{
   public:

      cDisplayVolumebar() : cDisplayItem() { visible = no; } 
      int groupOf()           { return groupVolume; }
      int isForegroundItem()  { return yes; }

      int draw();
};

class cDisplayTimebar : public cDisplayItem
{
   public:

      int groupOf()  { return groupChannel; }
      int draw();
};

class cDisplayProgressBar : public cDisplayItem
{
   public:

      int draw();
};

class cDisplaySymbol : public cDisplayItem
{
   public:

      int groupOf()  { return groupChannel; }
      int draw();
};

class cDisplayMenu : public cDisplayItem
{
   public:

      int draw();
};

class cDisplayMenuSelected : public cDisplayItem
{
   public:

      int draw();
};

class cDisplayMenuColumn : public cDisplayItem
{
   public:

      int groupOf()  { return groupMenu; }
      int draw();
};

class cDisplayMenuColumnSelected : public cDisplayItem
{
   public:

      int groupOf()  { return groupMenu; }
      int draw();
};

class cDisplayMenuEventColumn : public cDisplayItem
{
   public:

      int groupOf()  { return groupMenu; }
      int draw();
};

class cDisplayMenuEventColumnSelected : public cDisplayItem
{
   public:

      int groupOf()  { return groupMenu; }
      int draw();
};

class cDisplayPartingLine : public cDisplayItem
{
   public:

      int draw() { return cDisplayItem::draw(); }
};

class cDisplaySpectrumAnalyzer : public cDisplayItem
{
   public:

      int draw();
};

class cDisplaySysinfo : public cDisplayItem
{
   public:

      int draw();

   protected:

      int lastCpuLoad;
      unsigned long lastUsedMem;
      unsigned long lastUsedDisk;
};

//***************************************************************************
// Lists
//***************************************************************************
//***************************************************************************
// Theme Items
//***************************************************************************

class cDisplayItems : public cList<cDisplayItem>
{
   public:

      cDisplayItem* getItemByKind(Ts::ItemKind k);
      cDisplayItem* getItemById(const char* id);
};

//***************************************************************************
// Class cThemeSection
//  - represents a section of a theme
//***************************************************************************

class cThemeSection : public cListObject, cThemeService
{
   public:
      cThemeSection& operator= (const cThemeSection &ThemeSection)
	  {
		return *this;
	  };
      cThemeSection(string aName = "unnamed")       { name = aName; defaultsItem = 0; }
      ~cThemeSection()                              { if (defaultsItem) delete defaultsItem; }

      string getName()                              { return name; }
      cDisplayItems* getItems()                     { return &items; }

      cDisplayItem* getItemByKind(ItemKind k)       { return items.getItemByKind(k); }
      cDisplayItem* getItemById(const char* id)     { return items.getItemById(id); }

      cDisplayItem* First()                         { return items.First(); }
      cDisplayItem* Next(cDisplayItem* item)        { return items.Next(item); }

      void Ins(cDisplayItem* item, cDisplayItem* before = 0) 
      { items.Ins(item, before); item->setSection(this); }

      void Add(cDisplayItem* item, cDisplayItem* after = 0)  
      { items.Add(item, after); }

      cDisplayItem* getDefaultsItem()               { return defaultsItem; }

      void setDefaultsItem(cDisplayItem* item)  
      {  
         if (defaultsItem) 
            delete defaultsItem;

         defaultsItem = item;
      }

      int lookupVar(string name, string& value)
      { 
         value = "";

         map<string,string>::iterator iter = variables.find(name);

         if (iter != variables.end())
         {
            value = iter->second.c_str();
            return success;
         }

         if (varFiles.size())
         {
            string::size_type p;

            if ((p = name.find(".")) == string::npos)
               return fail;

            map<string,cVariableFile*>::iterator itFile = varFiles.find(name.substr(0, p));
            
            if (itFile != varFiles.end())
            {
               if (itFile->second->lookupVar(name, value) == success)
                  return success;
            }
         }

         return fail;
      }

      uint64_t getNextUpdateTime();
      int reset();
      int updateGroup(int group);

      void addVarFile(cDisplayItem* p) 
      { 
         cVariableFile* pp = (cVariableFile*)p;
         varFiles[pp->getName()] = pp;
      }

      int hasVarFile(const char* name)
      {  
         return varFiles.find(name) !=  varFiles.end();
      }
      
      map<string,cVariableFile*>* getVarFiles()
      {
         return &varFiles; 
      }

      int updateVariables();

      map<string,string> variables;

   protected:

      string name;
      cDisplayItems items;
      cDisplayItem* defaultsItem;  // Item holding default values
      map<string,cVariableFile*> varFiles;
};

//***************************************************************************
// Class cThemeSections
//    - holding all sections
//***************************************************************************

class cThemeSections : public cList<cThemeSection>
{
   public:

      cThemeSection* getSection(string name);
};

//***************************************************************************
// Class cGraphTFTTheme
//   - holding all Items included in the theme configuration file
//***************************************************************************

class cGraphTFTTheme : public cConfig<cThemeItem>, public cListObject, cThemeService
{
   public:

      cGraphTFTTheme();
      virtual ~cGraphTFTTheme()   { exit(); }

      // functions

      int init();
      int exit();
      int activate(int fdInotify);
      int deactivate(int fdInotify);
      int check(const char* theVersion);
      int load(const char* path);
      int checkViewMode();

      // get

      string getName()                     { return name; }
      string getThemeVersion()             { return themeVersion; } 
      string getSyntaxVersion()            { return syntaxVersion; }
      string getDir()                      { return dir; }
      string getStartImage()               { return startImage; }
      string getEndImage()                 { return endImage; }
      int getWidth()                       { return width; }
      int getHeight()                      { return height; }
      string getFontPath()                 { return fontPath; }
      int isInitialized()                  { return initialized; }

      // set

      void setName(string aValue)          { name = aValue; }

      void setThemeVersion(string aValue)  { themeVersion = aValue; }
      void setSyntaxVersion(string aValue) { syntaxVersion = aValue; }
      void setDir(string aValue)           { dir = aValue; }
      void setStartImage(string aValue)    { startImage = aValue; }
      void setEndImage(string aValue)      { endImage = aValue; }
      void setWidth(int aValue)            { width = aValue; }
      void setHeight(int aValue)           { height = aValue; }
      void setFontPath(string aValue)      { fontPath = aValue; }

      void AddMapItem(cDisplayItem* item, cDisplayItem* after = 0)
      { mapSection.Add(item, after); }

      // 

      void addNormalSection(string section);
      const char* getNormalMode(const char* modeName);
      const char* nextNormalMode(const char* modeName);
      const char* prevNormalMode(const char* modeName);
      char** getNormalSections()          { return normalModes; }
      int getNormalSectionsCount()        { return normalModesCount; }

      // the sections

      cThemeSection*  getSection(string name) { return sections.getSection(name); }
      cThemeSections* getSections()           { return &sections; }

      cThemeSection* FirstSection()              
      { return sections.First(); }

      cThemeSection* NextSection(cThemeSection* sect)
      { return sections.Next(sect); }

      string getPathFromImageMap(const char* name);

      void resetDefines() { defineCount = 0; clearIfdefs();}
      void clearIfdefs()  { while (!skipContent.empty()) skipContent.pop(); }
      int isSkipContent() { return !skipContent.empty() && skipContent.top(); }

      int lookupVar(string name, string& value) 
      { 
         value = "";

         map<string,string>::iterator iter;
         
         iter = menuVariables.find(name);

         if (iter != menuVariables.end())
         {
            value = iter->second.c_str();
            return success;            
         }

         iter = variables.find(name);

         if (iter != variables.end())
         {
            value = iter->second.c_str();
            return success;
         }

         return fail; 
      }

      // data

      string defines[100];
      int defineCount;
      std::stack<int> skipContent;

      map<string, string> variables;
      map<string, string> menuVariables;
      map<int, cDisplayItem*> inotifies;

   protected:

      // data

      int width;
      int height;
      string dir;
      string name;
      string themeVersion;
      string syntaxVersion;
      string startImage;
      string endImage;
      string fontPath;
      int initialized;
      cThemeSections sections;
      cThemeSection mapSection;
      char* normalModes[100];
      int normalModesCount;
};

//***************************************************************************
// Class cGraphTFTThemes
//    - holding all themes
//***************************************************************************

class cGraphTFTThemes : public cList<cGraphTFTTheme>
{
   public:

      cGraphTFTTheme* getTheme(string aTheme);

      static cGraphTFTTheme* theTheme;

   protected:
};

//***************************************************************************
// 
//***************************************************************************

typedef cGraphTFTThemes Thms;

//***************************************************************************
// External
//***************************************************************************

extern cGraphTFTThemes themes;

//***************************************************************************
#endif //__GTFT_THEME_H
