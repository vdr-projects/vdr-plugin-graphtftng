/**
 *  GraphTFT plugin for the Video Disk Recorder 
 * 
 *  theme.c - A plugin for the Video Disk Recorder
 *
 *  (c) 2004 Lars Tegeler, Sascha Volkenandt
 *  (c) 2006-2013 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 **/

#include <sys/inotify.h>

#include <theme.h>
#include <common.h>
#include <scan.h>

#include "setup.h"

//***************************************************************************
// The Theme
//***************************************************************************

cGraphTFTTheme* Thms::theTheme = 0;
cGraphTFTThemes themes;

//***************************************************************************
// Convert Channel-Name in Logo Path
//***************************************************************************

string VariableProvider::channelLogoPath(const char* channel, 
                                         const char* format, int classic)
{
   unsigned int s = 0;
   unsigned int d = 0;
   unsigned int len = strlen(channel);
   char* file = (char *)malloc(len+TB); 

   // remove '\/:*?"<>| character

   while (channel[s])
   {
      if (!strchr("/':*?\"<>\\", channel[s]))
         file[d++] = channel[s];
      
      s++;
   }

   file[d] = 0;

   // special handling fore some channels

   if (!rep(file, "D [0-9] \\- ...."))
   {
      free(file);
      file = strdup("DIREKT");
   }

   string path;

   if (classic)
       path = "columnimages/" + string(file) + "." 
          + ((format && *format) ? string(format) : "png");
   else
      path = string(file);

   free(file);

   tell(5, "converted logo name is '%s'", path.c_str());

   return path;
}

//***************************************************************************
// 
//***************************************************************************

const char* VariableProvider::splitFormatValue(const char* data, 
                                               char* value, char* format)
{
   const char* f = strchr(data, '/');
   
   if (f)
   {
      strncpy(value, data, f-data);
      value[f-data] = 0;
      strcpy(format, f+1);

      return value;
   }

   strcpy(value, data);
   *format = 0;

   return value;
}

//***************************************************************************
// Variable Of
// - returns the first variable name in expession
//  "{varTest} xx {varHallo}" -> varTest
//***************************************************************************

int VariableProvider::variableOf(string& name, const char* expression, char*& e)
{
   const char* s;

   tell(4, "variableOf '%s'", expression);

   if ((s = strchr(expression, '{')))
   {
      s++;

      if (!(e = (char *)strchr(s, '}')))
      {
         tell(0, "Parsing of [%s] failed, missing bracket '}'", expression);
         return fail;
      }

      // found expression

      name = string(expression, s-expression, e-s);
      tell(4, "variable '%s' found", name.c_str());

      return success;
   }

   return fail;
}

//***************************************************************************
// Evaluate Variable
//***************************************************************************

int VariableProvider::evaluate(string& buf, const char* var)
{
   const char* s;
   const char* e;
   string v;
   const char* p = var;
   char exp[1000+TB];
   int len;
   char tmp[1+TB];

   buf = "";

   tell(4, "evaluating '%s'", var);

   while ((s = strchr(p, '{')))
   {
      if (s > p)
         buf.append(p, s-p);
      
      if (!(e = strchr(p, '}')))
      {
         tell(0, "Parsing of [%s] failed, missing bracket '}'", var);
         return fail;
      }

      // found expression

      e--;
      len = std::min((int)(e-s), 1000);

      if (len < 1)
      {
         tell(0, "Parsing of [%s] failed, bracket missmatch", var);
         return fail;
      }

      strncpy(exp, s+1, len);
      exp[len] = 0;

      tell(4, "variable '%s' found", exp);

      if (*exp == '\\' && strlen(exp) > 1)
      {
         tmp[0] = (char)atoi(exp+1);
         tmp[1] = 0;
         v = tmp;

         tell(4, "found character expression '%s'", v.c_str());
      }
      else 
      {
         char fmt[100+TB];
         char val[255+TB];

         // split if expression contais format string
         // in a expression a '/' always start a format string or special function

         splitFormatValue(exp, val, fmt);
   
         tell(4, "expression '%s' with variable '%s' and format '%s'", 
              exp, val, fmt);

         // lookup variable

         if (lookupVariable(val, v, fmt) != success)
            return fail;

         tell(4, "I Variable '%s' evaluated, value is '%s'", val, v.c_str());

         string vv;

         if (strchr(v.c_str(), '{') && evaluate(vv, v.c_str()) == success)
         {
            tell(4, "II Variable '%s' evaluated, value is '%s'", v.c_str(), vv.c_str());
            v = vv;
         }

         // do the format stuff

         if (!Str::isEmpty(fmt) && strcasecmp(fmt, "toLogo") == 0)
            v = channelLogoPath(v.c_str(), 0, no);
      }
         
      buf.append(v);

      p = e+2;
   }

   buf.append(p);

   return success;
}

//***************************************************************************
// Calculate Expression
//   calc expressions like: "2 + 4"
//***************************************************************************

int VariableProvider::calcExpression(const char* expression)
{
   Scan* scan;
   int result = 0;
   char op[100]; *op = 0;
   
   if (!expression || !*expression)
      return 0;
   
   scan = new Scan(expression, no);

   // left expression

   if (scan->eat(yes) != success || !scan->isNum())
   {
      delete scan;
      return result;
   }

   result = scan->lastInt();

   while (scan->eat() == success && scan->isOperator())
   {
      strcpy(op, scan->lastIdent());
      
      if (scan->eat(yes) != success || !scan->isNum())
      {
         delete scan;
         return result;
      }

      result = calc(op, result, scan->lastInt());
   }

   delete scan;

   return result;
}

int VariableProvider::calc(const char* op, int left, int right)
{
   int result = left;

   if (*op == '+')
      result += right;
   else if (*op == '-')
      result -= right;
   else if (*op == '*')
      result *= right;
   else if (*op == ':')
      result /= right;
   
   return result;
}

//***************************************************************************
// cThemeService
//***************************************************************************

const char* cThemeService::items[] =
{
   "Include",
   "Theme",
   
   "Text",
   "Image",
   "ImageFile",
   "ImageDir",
   "Rectangle",
   "Timebar",

   "Message",
   "VolumeMuteSymbol",
   "Volumebar",

   "Menu",
   "MenuSelected",

   "MenuButtonRed",
   "MenuButtonGreen",
   "MenuButtonYellow",
   "MenuButtonBlue",

   "MenuButtonBackgroundRed",
   "MenuButtonBackgroundGreen",
   "MenuButtonBackgroundYellow",
   "MenuButtonBackgroundBlue",

   "MenuImageMap",

   "SpectrumAnalyzer",
   "PartingLine",
   "Sysinfo",
   "Background",
   "TextList",
   "Progressbar",

   "ClickArea",
   "MenuNavigationArea",
   "CalibrationCursor",

   "Column",
   "ColumnSelected",

   "EventColumn",
   "EventColumnSelected",

   "Defaults",
   "VariableFile",

   0
};

const char* cThemeService::toName(ItemKind aItem)
{
   if (!isValid(aItem))
      return "unknown";

   return items[aItem];
}

cThemeService::ItemKind cThemeService::toItem(const char* aName)
{
   for (int i = 0; items[i]; i++)
      if (strcasecmp(items[i], aName) == 0)
         return (ItemKind)i;

   return itemUnknown;
}

//***************************************************************************
// Translations
//***************************************************************************

cThemeService::Translation cThemeService::alignments[] =
{
   { algLeft,   "left"   },
   { algCenter, "center" },
   { algRight,  "right"  },
   { 0, 0 }
};

cThemeService::Translation cThemeService::scrollmodes[] =
{
   { smOff,      "off"     },
   { smMarquee,  "marquee" },
   { smTicker,   "ticker"  },
   { 0, 0 }
};

int cThemeService::toDenum(Translation t[], const char* value)
{
   for (int i = 0; t[i].name; i++)
   {
      if (strcasecmp(t[i].name, value) == 0)
         return t[i].denum;
   }

   return na;
}

//***************************************************************************
// Class cThemeItem
//***************************************************************************

cThemeSection* cThemeItem::currentSection = 0;
string cThemeItem::lineBuffer = "";
string cThemeItem::condition = "";

cThemeItem::cThemeItem()
{
   _item = itemUnknown;

   _x = "-1";
   _y = "-1";
   _width = "0";
   _height = "0";

   _bg_x = na;
   _bg_y = na;
   _image_map = 0;
   _stat_pic = 0;
   _stat_text=0;
   _stat_width = 0;
   _stat_height = 0;
   _stat_x = 0;
   _stat_y = 0;
   _bg_width = 0;
   _bg_height = 0;
   _overlay = no;
   _lines = 0;
   _start_line = "0";
   _line = 0;
   _size = 32;
   _switch = no;
   _align = Ts::algLeft;
   _align_v = 0;
   _delay=0;
   _foreground = no;
   _count=0;
   _spacing = 0;
   _yspacing = 5;
   _bar_height = 100;
   _bar_height_unit = iuPercent;
   _scroll = smOff;
   _scroll_count = 0;
   _dots = no;
   _permanent = no;
   _factor = 1;
   _aspect_ratio = no;
   _rotate = 0;             // { 0 - off; 1 - auto; }
   _fit = no;
   _value = "";
   _total = "";

   _menu_x = 0;
   _menu_y = 0;
   _menu_width = 0;
   _menu_height = 0;

   _color = "255:255:255:255";
   _bg_color = "0:0:0:255";
   _unit = "";
   _reference = "";
   _font = "graphTFT";
   _type = "";
   _format = "";
   _path = "";
   _path2 = "";
   _focus = "";
   _text = "";
   _number = na;
   _index = 0;
   _whipe_res = 20;
   _onClick = "";
   _onDblClick = "";
   _onUp = "";
   _onDown = "";
   _sectionInclude = "";
   _condition = "";
   _debug = "-";
   _id = "";
   _area = "";

   section = 0;
   pathCount = 0;
}

cThemeItem::~cThemeItem()
{
}

//***************************************************************************
// Parse one line => one Item
//***************************************************************************

bool cThemeItem::Parse(const char* s)
{
   cDisplayItem* dspItem;
   string tmp;
   string::size_type posA;
   string::size_type posB;
   int value;
   string toParse;

   // skip whitespace

   while (*s && (*s == ' ' || *s == '\t'))
      s++;

   // skip empty lines

   if (Str::isEmpty(s))
      return true;

   // append

   lineBuffer.append(s);

   // skip line and inline comments

   if ((posA = lineBuffer.find("//")) != string::npos)
   {   
      lineBuffer.erase(posA);
      
      if (Str::isBlank(lineBuffer.c_str()))
         return true;
   }
   
   // parse directives

   if ((posA = lineBuffer.find("#")) == 0)
   {
      parseDirectives(lineBuffer);
      lineBuffer.clear();
      return true;
   }

   if (Thms::theTheme->isSkipContent())
   {
      lineBuffer.clear();
      tell(2, "Skipping line due to directive [%.50s%s]",
           s, strlen(s) > 50 ? ".." : "");
      return true;
   }

   // parse variables

   if ((posA = lineBuffer.find("var ")) == 0)
   {
      string tmp = lineBuffer.substr(posA+4);
      parseVariable(tmp, currentSection);
      lineBuffer.clear();

      return true;
   }

   // parse condition

   if ((posA = lineBuffer.find("if ")) == 0)
   {
      condition = lineBuffer.substr(posA+3);
      lineBuffer.clear();

      return true;
   }
   else if (lineBuffer.find("endif") == 0)
   {
      condition = "";
      lineBuffer.clear();
      return true;
   }

   // check if it is a section start

   posA = lineBuffer.find("[");
   posB = lineBuffer.find("]", posA);
   
   if (posA == 0 && posB > 0 && posB != string::npos)
   {
      // section ...
      
      tmp = lineBuffer.substr(posA+1, posB-1);

      // append section

      currentSection = new cThemeSection(tmp);
      Thms::theTheme->getSections()->Add(currentSection);
      Thms::theTheme->addNormalSection(currentSection->getName());
      lineBuffer.clear();

      return true;
   }

   // line completed ?

   if (lineBuffer.find(";") == string::npos)
   {
      tell(5, "line buffer now [%s]", lineBuffer.c_str());
      return true;
   }

   // ----------------------------
   // 

   tell(4, "line completed, processing [%s]", toParse.c_str());
   toParse = lineBuffer;
   lineBuffer.clear();

   if (!currentSection)
      return true;

   // include section ...

   if (ParseVar(toParse, "Include", &tmp) == success)
   {
      if ((dspItem = newDisplayItem(itemSectionInclude)))
      {
         currentSection->Add(dspItem);
         dspItem->_sectionInclude = tmp;
      }

      return true;
   }

   // item ...

   if ((posA = toParse.find(" ")) == string::npos)
   {
      tell(0, "Ignoring invalid theme line [%.50s%s]",
           s, strlen(s) > 50 ? ".." : "");

      return true;
   }

   // parse item attributes ..

   _item = toItem(toParse.substr(0, posA).c_str());

   switch (_item)
   {
      case itemVarFile:
      {
         if ((dspItem = newDisplayItem(itemVarFile)))
         {
            dspItem->ParseText(toParse);
            dspItem->_item = _item;
            dspItem->setSection(currentSection);
            currentSection->addVarFile(dspItem);
            tell(2, "added variable file '%s', namespace '%s'", 
                 dspItem->Path2().c_str(), dspItem->Path().c_str());
         }
         
         return true;
      }

      case itemTheme:
      {
         ParseVarExt(toParse, "name", &tmp);
         Thms::theTheme->setName(tmp);
         ParseVarExt(toParse, "themeVersion", &tmp);
         Thms::theTheme->setThemeVersion(tmp);
         ParseVarExt(toParse, "syntaxVersion", &tmp);
         Thms::theTheme->setSyntaxVersion(tmp);
         ParseVarExt(toParse, "dir", &tmp);
         Thms::theTheme->setDir(tmp);
         ParseVarExt(toParse, "startImage", &tmp);
         Thms::theTheme->setStartImage(tmp);
         ParseVarExt(toParse, "endImage", &tmp);
         Thms::theTheme->setEndImage(tmp);

         if (ParseVarExt(toParse, "width", &value) == success)
            Thms::theTheme->setWidth(value);
         if (ParseVarExt(toParse, "height", &value) == success)
            Thms::theTheme->setHeight(value);
         if (ParseVarExt(toParse, "fontPath", &tmp) == success)
            Thms::theTheme->setFontPath(tmp);

         return true;
      }

      case itemDefaults:
      {
         currentSection->setDefaultsItem(newDisplayItem(_item));
         currentSection->getDefaultsItem()->ParseText(toParse);

         return true;
      }

      case itemUnknown:
      {
         tell(0, "Warning: Ignoring unknown theme item = [%s]", 
              toParse.substr(0, posA).c_str());

         return true;
      }
   }

   dspItem = newDisplayItem(_item);

   if (dspItem)
   {
      if (_item == itemMenuImageMap)
         Thms::theTheme->AddMapItem(dspItem);
      else
         currentSection->Add(dspItem);

      // copy default values

      if (currentSection->getDefaultsItem())
         *dspItem = *currentSection->getDefaultsItem(); // copy constructor
      else
         tell(2, "Info: No defaults for section '%s' defined", 
              currentSection->getName().c_str());

      // set/parse item properties

      dspItem->_item = _item;
      dspItem->setSection(currentSection);
      dspItem->ParseText(toParse);
      
      if (condition.size())
      {
         if (dspItem->_condition.size())
            dspItem->_condition += " && ";

         dspItem->_condition += condition;

         tell(3, "Attach condition '%s' to '%s', condition now '%s'", 
              condition.c_str(), dspItem->nameOf(), dspItem->_condition.c_str());
      }
   }

   return true;
}

//***************************************************************************
// Parse Variable
//***************************************************************************

int cThemeItem::parseVariable(string& toParse, cThemeSection* section)
{
   int menu = no;
   string name, value;
   Scan scan(toParse.c_str());

   scan.eat();
   
   if (!scan.isIdent())
   {
      tell(0, "Error: Invalid left value '%s' in '%s'",
           scan.lastIdent(), toParse.c_str());
      return fail;
   }

   // parse optional 'menu' key word

   if (strcmp(scan.lastIdent(), "menu") == 0)
   {
      menu = yes;
      scan.eat();

      if (!scan.isIdent())
      {
         tell(0, "Error: Invalid left value '%s' in '%s'",
              scan.lastIdent(), toParse.c_str());
         return fail;
      }
   }
  
   // name

   name = scan.lastIdent();

   scan.eat();   

   // assignment

   if (!scan.isOperator() || scan.lastIdent()[0] != '=')
   {
      tell(0, "Error: Invalid operator '%s' in '%s', '=' expected", 
           scan.lastIdent(), toParse.c_str());
      return fail;
   }

   scan.eat();

   if (!scan.isString() && scan.isIdent())
   {
      tell(0, "Error: Invalid right value '%s' in '%s'",
           scan.lastIdent(), toParse.c_str());
      return fail;
   }

//   evaluate(value, scan.lastIdent());   // #jw value = scan.lastIdent();
   value = scan.lastIdent();
   
   if (section)
   {
      tell(3, "adding section variable '%s' with '%s' to '%s'", 
           name.c_str(), value.c_str(), section->getName().c_str());
      section->variables[name] = value;
   }
   else
   {
      tell(3, "adding theme variable '%s' with '%s' %s",
           name.c_str(), value.c_str(), menu ? "mode menu" : "");

      if (menu)
         Thms::theTheme->menuVariables[name] = value;
      else
         Thms::theTheme->variables[name] = value;
   }
   
   return success;
}

//***************************************************************************
// Parse Directive
//***************************************************************************

int cThemeItem::parseDirectives(string& toParse)
{
   string exeption;
   
   if (ParseDirective(toParse, "#define", &exeption) == success 
       && exeption.length())
   {
      tell(1, "Adding define '%s'", exeption.c_str());
      Thms::theTheme->defines[Thms::theTheme->defineCount] = exeption;
      Thms::theTheme->defineCount++;
      
      return success;
   }

   else if (ParseDirective(toParse, "#endif", &exeption) == success)
   {
      if (Thms::theTheme->skipContent.empty())
         tell(0, "Warning: Ignoring unexpected '#endif'");
      else
         Thms::theTheme->skipContent.pop();

      tell(2, "Swiching parser '%s' ('#endif' found)",
           Thms::theTheme->isSkipContent() ? "off" : "on");
      
      return success;
   }

   else if (ParseDirective(toParse, "#else", &exeption) == success)
   {
      int actualSkip = Thms::theTheme->isSkipContent();

      if (Thms::theTheme->skipContent.empty())
         tell(0, "Warning: Ignoring unexpected '#else'");
      else
         Thms::theTheme->skipContent.top() = !actualSkip;

      tell(2, "Swiching parser '%s' [from '%s'] ('#else' found)", 
           Thms::theTheme->isSkipContent() ? "off" : "on",
           actualSkip ? "off" : "on");

      return success;
   }

   else if ((ParseDirective(toParse, "#ifdef", &exeption) == success
             || ParseDirective(toParse, "#ifndef", &exeption) == success) 
            && exeption.length())
   {
      int actualSkip = Thms::theTheme->isSkipContent();
      int negate = no;

      if (toParse.find("#ifndef") == 0)
         negate = yes;

      tell(4, "Found '%s %s'", negate ? "#ifndef" : "#ifdef", exeption.c_str());

      int skip = negate ? no : yes;

      for (int i = 0; i < Thms::theTheme->defineCount; i++) 
      {
         if (Thms::theTheme->defines[i] == exeption)
         {
            tell(4, "Define '%s' is set, switching parser '%s'", 
                 exeption.c_str(), negate ? "off" : "on");

            skip = negate ? yes : no;

            break;
         }
      }

      // aus bleibt aus (bei verschachtelten ifdef)

      skip = skip || actualSkip;

      Thms::theTheme->skipContent.push(skip);

      tell(2, "Parser now '%s' due to '%s' '%s'", 
           Thms::theTheme->isSkipContent() ? "off" : "on",
           negate ? "#ifndef" : "#ifdef",
           exeption.c_str());

      return success;
   }

   return ignore;
}

//***************************************************************************
// 
//***************************************************************************

cDisplayItem* cThemeItem::newDisplayItem(int item)
{
   cDisplayItem* newItem = 0;

   switch (item)
   {
      case itemMenuImageMap: 
      case itemDefaults:
      case itemMenuNavigationArea:
      case itemSectionInclude:      newItem = new cDisplayItem();                break;

      case itemText:                newItem = new cDisplayText();                break;
      case itemTextList:            newItem = new cDisplayTextList();            break;
      case itemProgressbar:         newItem = new cDisplayProgressBar();         break;
      case itemRectangle:           newItem = new cDisplayRectangle();           break;
      case itemImage:               newItem = new cDisplayImage();               break;
      case itemImageFile:           newItem = new cDisplayImageFile();           break;
      case itemImageDir:            newItem = new cDisplayImageDir();            break;
      case itemCalibrationCursor:   newItem = new cDisplayCalibrationCursor();   break;
      case itemMessage:             newItem = new cDisplayMessage();             break;

      case itemMenu:                newItem = new cDisplayMenu();                break;
      case itemMenuSelected:        newItem = new cDisplayMenuSelected();        break;
      case itemColumn:              newItem = new cDisplayMenuColumn();          break;
      case itemColumnSelected:      newItem = new cDisplayMenuColumnSelected();  break;
      case itemEventColumn:         newItem = new cDisplayMenuEventColumn();          break;
      case itemEventColumnSelected: newItem = new cDisplayMenuEventColumnSelected();  break;

      case itemSpectrumAnalyzer:    newItem = new cDisplaySpectrumAnalyzer();    break;
      case itemPartingLine:         newItem = new cDisplayPartingLine();         break;
      case itemSysinfo:             newItem = new cDisplaySysinfo();             break;
      case itemBackground:          newItem = new cDisplayBackground();          break;

      case itemVolumeMuteSymbol:    newItem = new cDisplayVolumeMuteSymbol();    break;
      case itemVolumebar:           newItem = new cDisplayVolumebar();           break;

      case itemTimebar:             newItem = new cDisplayTimebar();             break;
      case itemVarFile:             newItem = new cVariableFile();               break;

      case itemMenuButtonRed:
      case itemMenuButtonGreen:
      case itemMenuButtonYellow:
      case itemMenuButtonBlue:
         newItem = new cDisplayMenuButton();
         break;

      case itemMenuButtonBackgroundRed:
      case itemMenuButtonBackgroundGreen:
      case itemMenuButtonBackgroundYellow:
      case itemMenuButtonBackgroundBlue:
         newItem = new cDisplayMenuButtonBackground();
         break;
   };

   if (newItem)
      newItem->_item = item;

   return newItem;
}

//***************************************************************************
// Parse Properties
//***************************************************************************

int cThemeItem::ParseText(string toParse)
{
   ParseVar(toParse, "id", &_id);
   ParseVar(toParse, "area", &_area);

   ParseVarColor(toParse, "color", &_color);
   ParseVarColor(toParse, "bg_color", &_bg_color);

   ParseVar(toParse, "x", &_x);
   ParseVar(toParse, "y", &_y);
   ParseVar(toParse, "width", &_width);
   ParseVar(toParse, "height", &_height);

   ParseVar(toParse, "overlay", &_overlay);
   ParseVar(toParse, "lines", &_lines);
   ParseVar(toParse, "start_line", &_start_line);
   ParseVar(toParse, "line", &_line);
   ParseVar(toParse, "font", &_font);
   ParseVar(toParse, "size", &_size);
   ParseVar(toParse, "text", &_text);
   ParseVar(toParse, "switch", &_switch);         // bool
   ParseVar(toParse, "align_v", &_align_v);
   ParseVar(toParse, "align", &_align, alignments);
   ParseVar(toParse, "focus", &_focus);
   ParseVar(toParse, "type", &_type);
   ParseVar(toParse, "format", &_format);
   ParseVarTime(toParse, "delay", &_delay);
   ParseVar(toParse, "foreground", &_foreground);
   ParseVar(toParse, "count", &_count);
   ParseVar(toParse, "image_map", &_image_map);   // bool
   ParseVar(toParse, "stat_pic", &_stat_pic);     // bool
   ParseVar(toParse, "stat_text", &_stat_text);   // bool
   ParseVar(toParse, "stat_width", &_stat_width);
   ParseVar(toParse, "stat_height", &_stat_height);
   ParseVar(toParse, "stat_x", &_stat_x);
   ParseVar(toParse, "stat_y", &_stat_y);
   ParseVar(toParse, "bg_x", &_bg_x);
   ParseVar(toParse, "bg_y", &_bg_y);
   ParseVar(toParse, "bg_width", &_bg_width);
   ParseVar(toParse, "bg_height", &_bg_height);
   ParseVar(toParse, "number", &_number);
   ParseVar(toParse, "spacing", &_spacing);
   ParseVar(toParse, "yspacing", &_yspacing);
   ParseVar(toParse, "scroll", &_scroll, scrollmodes);
   ParseVar(toParse, "factor", &_factor);
   ParseVar(toParse, "unit", &_unit);
   ParseVar(toParse, "reference", &_reference);
   ParseVar(toParse, "scroll_count", &_scroll_count);
   ParseVar(toParse, "dots", &_dots);
   ParseVar(toParse, "permanent", &_permanent);
   ParseVar(toParse, "value", &_value);
   ParseVar(toParse, "total", &_total);

   ParseVar(toParse, "menu_x", &_menu_x);
   ParseVar(toParse, "menu_y", &_menu_y);
   ParseVar(toParse, "menu_width", &_menu_width);
   ParseVar(toParse, "menu_height", &_menu_height);

   ParseVar(toParse, "fit", &_fit);
   ParseVar(toParse, "aspect_ratio", &_aspect_ratio);
   ParseVar(toParse, "rotate", &_rotate);

   ParseVar(toParse, "whipe_res", &_whipe_res);
   ParseVar(toParse, "on_click", &_onClick);
   ParseVar(toParse, "on_dblclick", &_onDblClick);
   ParseVar(toParse, "on_up", &_onUp);
   ParseVar(toParse, "on_down", &_onDown);

   ParseVar(toParse, "name", &_path);
   ParseVar(toParse, "file", &_path2);
   ParseVar(toParse, "path2", &_path2);
   ParseVar(toParse, "pathON", &_path);
   ParseVar(toParse, "pathOFF", &_path2);

   ParseVar(toParse, "condition", &_condition);

   ParseVar(toParse, "debug", &_debug);

   // some properties with special handling

   string tmp;

   if (ParseVar(toParse,"bar_height", &tmp) == success)
   {
      _bar_height = atoi(tmp.c_str());

      if (strchr(tmp.c_str(), '%'))
         _bar_height_unit = iuPercent;
      else
         _bar_height_unit = iuAbsolute;
   }

   if (ParseVar(toParse, "path", &_path) == success)
   {
      string::size_type e, s;
      e = s = 0;

      do
      {
         e = _path.find(':', s);

         pathList[pathCount].configured = _path.substr(s, e == string::npos ? _path.length()-s : e-s);
         pathList[pathCount].curNum = na;
         pathList[pathCount].last = "";
         pathCount++;

         s = e+1;

      } while (s > 0 && pathCount < maxPathCount);
   }

   _text = replaceChar(_text, '@', '\n');
   
   return success;
}

//***************************************************************************
// Search via translation list
//***************************************************************************

int cThemeItem::ParseVar(string toParse, string name, int* value, Translation* t)
{
   string val;
   int status;
   int denum;

   if ((status = ParseVar(toParse, name, &val)) == success)
   {
      denum = cThemeService::toDenum(t, val.c_str());

      if (denum == na)
      {
         tell(0, "Error: Unexpected value '%s' for '%s'",
              val.c_str(), name.c_str());

         denum = 0;  // 0 -> always the default
      }

      *value = denum;
   }  

   return status;
}

//***************************************************************************
// Search the int parameter
//***************************************************************************

int cThemeItem::ParseVar(string toParse, string name, int* value)
{
   string val;
   int status;

   if ((status = ParseVar(toParse, name, &val)) == success)
   {
      if (val == "yes" || val == "true")
         *value = 1;
      else if (val == "no" || val == "false")
         *value = 0;
      else
         *value = atoi(val.c_str());
   }

   return status;
}

//***************************************************************************
// Search the time parameter
//***************************************************************************

int cThemeItem::ParseVarTime(string toParse, string name, uint64_t* value)
{
   string val;
   int status;

   if ((status = ParseVar(toParse, name, &val)) == success)
   {
      if (val.find("ms") != string::npos)
         *value = atoi(val.c_str());
      else
         *value = atoi(val.c_str()) * 1000;
   }

   return status;
}

//***************************************************************************
// Lookup Variable
//***************************************************************************

int cThemeItem::lookupVariable(const char* name, string& value, const char* fmt)
{
   int status = fail;

   tell(4, "lookup variable '%s' in '%s'", name, 
        section ? section->getName().c_str() : "theme");

   if (section)
      status = section->lookupVar(name, value);

   if (status != success)
      status = Thms::theTheme->lookupVar(name, value);

   if (status == success)
      tell(4, "Found variable '%s' with value '%s' in '%s'", 
           name, value.c_str(), section ? section->getName().c_str() : "theme");

   return status;
}

//***************************************************************************
// Set Variable
//***************************************************************************

int cThemeItem::setVariable(const char* name, int value)
{
   char v[50];
   string tmp;

   sprintf(v, "%d", value);

   if (section && section->lookupVar(name, tmp) == success)
   {
      section->variables[name] = v;
      return success;
   }
   else if (Thms::theTheme->lookupVar(name, tmp) == success)
   {
      Thms::theTheme->variables[name] = v;
      return success;
   }
   
   return fail;
}

//***************************************************************************
// Pase Variables
//***************************************************************************

int cThemeItem::ParseVarExt(string toParse, string name, int* value)
{
   string v, p;

   if (ParseVar(toParse, name, &v) != success)
      return fail;
   
   if (evaluate(p, v.c_str()) != success)
      return fail;

   *value = atoi(p.c_str());
      
   return success;
}

int cThemeItem::ParseVarExt(string toParse, string name, string* value)
{
   string v;

   if (ParseVar(toParse, name, &v) != success)
      return fail;

   if (evaluate(*value, v.c_str()) != success)
      return fail;

   tell(4, "found var '%s' with value '%s'", 
        name.c_str(), value->c_str());

   return success;
}

int cThemeItem::ParseVarColor(string toParse, string name, string* value)
{
   char* temp;
   int red, green, blue, alpha = na;

   int bg = strncmp(name.c_str(), "bg_", 3) == 0;  // background color expected?

   // parse alpha channel

   ParseVar(toParse, bg ? "bg_transparent" : "transparent", &alpha);
   ParseVar(toParse, bg ? "bg_alpha" : "alpha", &alpha);

   // parse new style color parameter
   
   if (ParseVar(toParse, name, value) != success)
   {
      // parse old style color parameter

      if (ParseVar(toParse, bg ? "bg_red"   : "red", &red) +
          ParseVar(toParse, bg ? "bg_green" : "green", &green) +
          ParseVar(toParse, bg ? "bg_blue"  : "blue", &blue) == success)
      {
         asprintf(&temp, "%d:%d:%d:%d", red, green, blue, alpha);
         *value = temp;
         free(temp);

         return success;
      }
   }

   // it's allowed to configure separate alpha value

   if (alpha != na)
   {
      t_rgba rgba;

      str2rgba(value->c_str(), rgba);
      asprintf(&temp, "%d:%d:%d:%d", rgba[rgbR], rgba[rgbG], rgba[rgbB], alpha);
      *value = temp;
      free(temp);
   }

   return success;
}

//***************************************************************************
// Search the string parameter
//***************************************************************************

int cThemeItem::ParseVar(string toParse, string name, string* value)
{
   string::size_type posA, posB, end = 0;

   name += "=";

   if ((posA = toParse.find("," + name)) == string::npos)
      if ((posA = toParse.find(" " + name)) == string::npos)
         if ((posA = toParse.find(name)) != 0)
            return fail;

   if (posA)
      posA++;

   posB = posA;

   while (posB < toParse.length())
   {
      if ((end = toParse.find(",", posB)) == string::npos)
         if ((end = toParse.find(";", posB)) == string::npos)
            return fail;

      // if at first pos or without mask sign, 
      // then we have found the end of the item
      // -> break the loop

      if (end == 0)                           // wenn ","   -> fertig
         break;

      if (toParse[end-1] != '\\')             // wenn "x,"  -> fertig
         break;

      if (end > 1 && toParse[end-2] == '\\')  // wenn "\\," -> fertig
         break;

      // => "\," -> nicht fertig
      // => search again behind the ',' or ';' sign

      posB = end+1;
   }

   *value = toParse.substr(posA + name.size(), end-posA-name.size());

   // de mask '\' sign

   if ((value->find("\\", 0)) != string::npos)
   {
      char* buf; char* s; char* d;

      asprintf(&buf, "%s", value->c_str());

      s = d = buf;

      while (*s)
      {
         if (*s != '\\' || *(s+1) == '\\')
            *d++ = *s;

         s++;
      }
      
      *d = 0;

      tell(5, "got '%s' build '%s'", value->c_str(), buf);

      *value = buf;
      free(buf);
   }

   return success;
}

//***************************************************************************
// Parse Directive
//***************************************************************************

int cThemeItem::ParseDirective(string toParse, string name, string* value)
{
   string::size_type posA;

   *value = "";

   if ((posA = toParse.find(name)) != 0)
      return fail;

   if (toParse.length() > posA + name.size() + 1)
      *value = toParse.substr(posA + name.size() + 1);

   return success;
}

//***************************************************************************
// Class cThemeSections
//***************************************************************************

//***************************************************************************
// Get Section By Name
//***************************************************************************

cThemeSection* cThemeSections::getSection(string name)
{
   for (cThemeSection* p = First(); p; p = Next(p)) 
      if (p->getName() == name)
         return p;

   return 0;
}

//***************************************************************************
// Class cThemeSection
//***************************************************************************

uint64_t cThemeSection::getNextUpdateTime()
{
   uint64_t next = msNow() + SECONDS(300); // 5 minutes
   cDisplayItem* pNext = 0;

   // search next (earlyast) drawing time

   for (cDisplayItem* p = First(); p; p = Next(p)) 
   {
      if (p->getNextDraw() && p->getNextDraw() < next)
      {
         uint64_t drawIn = p->getNextDraw() - msNow();

         tell(2, "setting next for '%s' in (%ld ms) [%s/%s]",
              p->nameOf(), drawIn, p->Debug().c_str(), p->Text().c_str());

         pNext = p;
         next = p->getNextDraw();
      }
   }

   if (pNext)
   {
      int64_t updateIn = next - msNow();

      if (updateIn < 0)
         updateIn = 0;

      int64_t s = updateIn/1000; 
      int64_t us = updateIn%1000;

      tell(2, "schedule next, nearest item is '%s'[%s] in %ld,%03ld seconds",
           pNext->nameOf(), pNext->Debug().c_str(), s, us);
   }

   return next < msNow() ? msNow() : next;
}

int cThemeSection::updateGroup(int group)
{
   tell(3, "update item group (%d)", group);

   for (cDisplayItem* p = First(); p; p = Next(p))
   {
      if (p->groupOf() & group)
      {
         p->reset();
         p->setNextDraw();

         // schedule all of my area also
         
         if (p->Area() != "")
         {
            for (cDisplayItem* pa = First(); pa; pa = Next(pa))
            {
               if (pa->Area() == p->Area())
               {
                  pa->reset();
                  pa->setNextDraw();
               }
            }
         }
      }
   }

   return done;
}

int cThemeSection::reset()
{
   tell(3, "reset items");

   for (cDisplayItem* p = First(); p; p = Next(p)) 
      p->reset();

   return done;
}

int cThemeSection::updateVariables()
{
   map<string,cVariableFile*>::iterator it;
   
   for (it = varFiles.begin(); it != varFiles.end(); it++)
      it->second->parse();

   return done;
}

//***************************************************************************
// Class cDisplayItems
//***************************************************************************
//***************************************************************************
// Get Item By Kind
//***************************************************************************

cDisplayItem* cDisplayItems::getItemByKind(Ts::ItemKind type)
{
   for (cDisplayItem* p = First(); p; p = Next(p)) 
   {
      if (p->Item() == type)
         return p;
   }

   return 0;
}

//***************************************************************************
// Get Item By ID  
//   id is configured in theme file, otherwise it is na
//***************************************************************************

cDisplayItem* cDisplayItems::getItemById(const char* id)
{
   for (cDisplayItem* p = First(); p; p = Next(p)) 
   {
      if (p->Id() == id)
         return p;
   }

   return 0;
}

//***************************************************************************
// Class GraphTFTTheme
//***************************************************************************

cGraphTFTTheme::cGraphTFTTheme()
{ 
   initialized = no; 

   width = 720;
   height = 576;
   memset(normalModes, 0, sizeof(normalModes));
   normalModesCount = 0; 
   fontPath = "";
   variables.clear();
   menuVariables.clear();

   cThemeItem::currentSection = 0;
   cThemeItem::lineBuffer = "";
   cThemeItem::condition = "";

   resetDefines();
}

//***************************************************************************
// Init
//***************************************************************************

int cGraphTFTTheme::init()
{
   cThemeSection* s;
   cThemeSection* sec;
   cDisplayItem* newItem;
   cDisplayItem* item;
   cDisplayItem* p;
   cDisplayItem* background;

   if (initialized)
      exit();

   // loop over sections 
   
   for (s = FirstSection(); s; s = NextSection(s))
   {
      // evaluate includes ...

      // loop over sections items

      tell(4, "Section: '%s'", s->getName().c_str());

      for (p = s->First(); p; p = s->Next(p)) 
      {
         // add items of 'included' sections

         tell(4, "Item: '%s'", Ts::toName((cThemeService::ItemKind)p->Item()));

         if (p->Item() == itemSectionInclude && (sec = getSection(p->SectionInclude())))
         {
            tell(4, "Icluding section: '%s'", sec->getName().c_str());
 
            map<string,cVariableFile*>::iterator it;
            
            for (it = sec->getVarFiles()->begin(); it != sec->getVarFiles()->end(); it++)
            {
               if (!s->hasVarFile(it->second->getName()))
                  s->addVarFile(it->second);
            }

            // include items

            for (item = sec->First(); item; item = sec->Next(item))
            {
               tell(4, "Including Item: '%s'", Ts::toName((cThemeService::ItemKind)item->Item()));

               if (item->Item() >= itemBegin
                   && (newItem = cThemeItem::newDisplayItem(item->Item())))
               {
                  *newItem = *item;           // copy constructor
                  s->Ins(newItem, p);         // s->First());
               }
            }

            // include variables

            map<string,string>::iterator iter;

            for (iter = sec->variables.begin(); iter != sec->variables.end(); ++iter)
               s->variables[iter->first] = iter->second;
         }
      }

      // assign background item ...

      background = 0;

      // loop over sections items

      for (p = s->First(); p; p = s->Next(p)) 
      {
         // detect/assign background
         //    only working properly if background is the first 
         //    item in the theme section!
      
         if (p->Item() == itemBackground)
         {
            if (background)
               tell(1, "Warning: in theme section '%s' cascading "
                    "background items detected", s->getName().c_str());

            background = p;
         }
         else
            p->setBackgroundItem(background);
      }
   }

   int ex = 0, exs = 0;
   int ey = 0;

   // init image map
   // init column positions
   // init ...

   // loop over all sections

   for (s = FirstSection(); s; s = NextSection(s)) 
   {
      int colNumber = 0;

      // loop over items of this section

      ex = exs = 0;
      ey = 0;

      for (p = s->First(); p; p = s->Next(p)) 
      {
         cDisplayItem* dspItem = p;

         if (!dspItem)
            continue;

         // calculate y position of the next text item

         if (dspItem->Item() == itemText)
         {
            if (dspItem->Y() == na)
               dspItem->setY(ey);
            else
               ey = dspItem->Y();

            if (dspItem->Height())
               ey += dspItem->Height() + dspItem->YSpacing();
         }

         // calculate x position of the column items

         if (dspItem->Item() == itemEventColumn)
             // ||  dspItem->Item() == itemColumn)
         {
            if (dspItem->Number() == na)
               dspItem->Number(colNumber);

            if (dspItem->X() == na)
               dspItem->setX(ex);                                  // not configured, take calculated
            else if (dspItem->X() < na && ex + dspItem->X() > 0)
               dspItem->setX(ex + dspItem->X());                   // go back
            else
               ex = dspItem->X();                                  // take configured value

            if (dspItem->Width())
               ex += dspItem->Width() + dspItem->Spacing();        // calc next x
            else 
               dspItem->setWidth(Thms::theTheme->getWidth() - dspItem->X());
         }

         if (dspItem->Item() == itemEventColumnSelected)
             //    || dspItem->Item() == itemColumnSelected)
         {
            if (dspItem->Number() == na)
               dspItem->Number(colNumber);

            if (dspItem->X() == na)
               dspItem->setX(exs);                                 // not configured, take calculated
            else if (dspItem->X() < na && exs + dspItem->X() > 0)
               dspItem->setX(exs + dspItem->X());                  // go back
            else
               exs = dspItem->X();                                 // take configured value
         
            if (dspItem->Width())
               exs += dspItem->Width() + dspItem->Spacing();       // calc next x
            else 
               dspItem->setWidth(Thms::theTheme->getWidth() - dspItem->X());
         }

         if (dspItem->Item() == itemEventColumn)
         {
            // ||   dspItem->Item() == itemColumn)
            colNumber++;
         }

         dspItem->init();
      }
   }

   // at leat reset all items

   for (s = FirstSection(); s; s = NextSection(s)) 
      s->reset();

   initialized = yes;

   return success;
}

int cGraphTFTTheme::activate(int fdInotify)
{
   cThemeSection* s;
   cDisplayItem* p;

   inotifies.clear();

   if (fdInotify != na)
   {
      // loop over all sections
      
      for (s = FirstSection(); s; s = NextSection(s)) 
      {
         // loop over items of this section
         
         for (p = s->First(); p; p = s->Next(p)) 
         {
            // add inotify watch
            
            if (p->Item() == itemImageFile)
            {
               if (p->Path().length())
               {
                  int wd = inotify_add_watch(fdInotify, p->Path().c_str(), IN_CREATE | IN_MODIFY); 

                  tell(0, "Adding inotify watch for '%s'", p->Path().c_str());
                  
                  if ((wd = -1))
                  {
                     tell(0, "Adding inotify watch for '%s' failed, %m", p->Path().c_str());
                     continue;
                  }

                  inotifies[wd] = p;
               }
            }
         }
      }
   }

   return success;
}

int cGraphTFTTheme::deactivate(int fdInotify)
{
   map<int,cDisplayItem*>::iterator iter;

   for (iter = inotifies.begin(); iter != inotifies.end(); iter++)
      inotify_rm_watch(fdInotify, iter->first);
   
   inotifies.clear();

   return success;
}

int cGraphTFTTheme::checkViewMode()
{
   // check normal view

   int i = 0;

   while (normalModes[i])
   {
      if ((GraphTFTSetup.storeNormalMode && (normalModes[i] == GraphTFTSetup.normalMode))
       || (!GraphTFTSetup.storeNormalMode && (normalModes[i] == GraphTFTSetup.originalNormalMode)))
         break;
      
      i++;
   }

   if (!normalModes[i])
   {
      GraphTFTSetup.normalMode = "Standard";
      GraphTFTSetup.storeNormalMode = true;
      GraphTFTSetup.originalNormalMode = "";
   }

   if (GraphTFTSetup.storeNormalMode)
      tell(0, "normal mode now '%s'", GraphTFTSetup.normalMode.c_str());
   else
      tell(0, "normal mode now '%s'", GraphTFTSetup.originalNormalMode.c_str());
   
   return done;
}

//***************************************************************************
// Exit
//***************************************************************************

int cGraphTFTTheme::exit()
{
   int i = 0;
   cThemeSection* s;

   if (!initialized)
      return done;

   cThemeItem::currentSection = 0;
   cThemeItem::lineBuffer = "";
   cThemeItem::condition = "";

   tell(3, "destroy theme '%s'", getName().c_str());

   for (s = FirstSection(); s; s = NextSection(s))
      s->getItems()->Clear();
   
   sections.Clear();

   while (normalModes[i] && i < 100)
      free(normalModes[i++]);

   // delete of this object's is done by 
   // cConfig before new Load() or in destructor!

   if(Thms::theTheme)
   {
       Thms::theTheme->resetDefines();
   }

   initialized = no;
   
   return done;
}

//***************************************************************************
// Check
//***************************************************************************

int cGraphTFTTheme::check(const char* theVersion)
{
   // check if the theme fit to current version
      
   if (strcmp(syntaxVersion.c_str(), theVersion) != 0)
   {
      tell(0, "Warning: Version of themefile syntax "
           "does not match, found '%s' instead of '%s'", 
           syntaxVersion.c_str(), theVersion);
      return fail;
   }

   return success;
}

//***************************************************************************
// Load
//***************************************************************************

int cGraphTFTTheme::load(const char* path)
{
   exit();
   Load(path);
   
   return init();
}

//***************************************************************************
// Add Normal Section
//***************************************************************************

void cGraphTFTTheme::addNormalSection(string sectionName)
{
   if (normalModesCount >= 100)
      return ;

   if (sectionName.find("Normal") != 0 || sectionName.length() <= strlen("Normal"))
      return ;

   if (sectionName == "NormalRadio")
      return ;

   if (sectionName == "NormalTV")
      asprintf(&normalModes[normalModesCount], "%s", 
               "Standard");
   else
      asprintf(&normalModes[normalModesCount], "%s", 
               sectionName.substr(strlen("Normal")).c_str());

   normalModesCount++;
}

//***************************************************************************
// Is Normal Mode Section
//***************************************************************************

const char* cGraphTFTTheme::getNormalMode(const char* modeName)
{
   for (int i = 0; i < normalModesCount; i++)
   {
      if (strcasecmp(normalModes[i], modeName) == 0)
         return normalModes[i];
   }

   return 0;
}

const char* cGraphTFTTheme::nextNormalMode(const char* modeName)
{
   int i;

   for (i = 0; i < normalModesCount; i++)
   {
      if (strcasecmp(normalModes[i], modeName) == 0)
         break;
   }

   if (++i < normalModesCount)
      return normalModes[i];
   
   return normalModes[0];
}

const char* cGraphTFTTheme::prevNormalMode(const char* modeName)
{
   int i;

   for (i = normalModesCount-1; i >= 0; i--)
   {
      if (strcasecmp(normalModes[i], modeName) == 0)
         break;
   }

   if (--i >= 0)
      return normalModes[i];
   
   return normalModes[normalModesCount-1];
}

//***************************************************************************
// Get Path From ImageMap
//***************************************************************************

string cGraphTFTTheme::getPathFromImageMap(const char* aName)
{
   const char* name = aName;

   // skip leading blanks and numbers

   while (*name && (isdigit(*name) || *name == ' '))
      name++;

   tell(3, "checking imagemap for menu entry '%s'", name);

   for (cDisplayItem* p = mapSection.First(); p; p = mapSection.Next(p)) 
   {
      if (p->Item() == itemMenuImageMap)
      {
         if (strcmp(trVDR(p->Path().c_str()), name) == 0)
         {
            tell(3, "MenuImageMap: Picture for '%s' is '%s'", aName, p->Path2().c_str());
            return p->Path2();
         }
      }
   }

   return "";
}

//***************************************************************************
// Calss cGraphTFTThemes
//***************************************************************************
//***************************************************************************
// Get Theme
//***************************************************************************

cGraphTFTTheme* cGraphTFTThemes::getTheme(string aTheme)
{
   cGraphTFTTheme* t;

   tell(3, "looking for theme '%s'", aTheme.c_str());

   for (t = First(); t; t = Next(t))
      if (t->getName() == aTheme)
         return t;

   return 0;
}
