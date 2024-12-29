/*
 *  GraphTFT plugin for the Video Disk Recorder
 *
 * dspitems.c
 *
 * (c) 2007-2015 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 */

//***************************************************************************
// Includes
//***************************************************************************

#include <sstream>

#include <sysinfo.h>
#include <theme.h>
#include <display.h>
#include <scan.h>
#include <span.h>
#include <setup.h>

#include <libexif/exif-data.h>

const int maxGranularity = 100;

//***************************************************************************
// Init Statics
//***************************************************************************

Renderer* cDisplayItem::render = 0;
cGraphTFTDisplay* cDisplayItem::vdrStatus = 0;
int cDisplayItem::forceDraw = yes;
uint64_t cDisplayItem::nextForce = 0;
cDisplayItem* cDisplayItem::selectedItem = 0;

//***************************************************************************
// cDisplayItem
//***************************************************************************

void cDisplayItem::scheduleForce(uint64_t aTime)
{
   if (!nextForce || nextForce > aTime)
   {
      nextForce = aTime;

      tell(1, "schedule force in (%ldms)",
           nextForce - msNow());
   }
}

void cDisplayItem::scheduleDrawAt(uint64_t aTime)
{
   // if (aTime < nextDraw || nextDraw < msNow())
   {
      nextDraw = aTime;

      tell(2, "schedule next draw of '%s'[%s] in (%ldms)",
           nameOf(), Debug().c_str(),
           nextDraw - msNow());
   }
}

void cDisplayItem::scheduleDrawIn(int aTime)
{
   uint64_t at = aTime + msNow();

   // the maximal redraw granularity is 500ms (maxGranularity), due to this
   //   adjust to next full step

   at = round((double)((double)at / maxGranularity)) * maxGranularity;

   scheduleDrawAt(at);
}

void cDisplayItem::scheduleDrawNextFullMinute()
{
   uint64_t ms = SECONDS(((time(0)/60 +1) * 60) - time(0));

   scheduleDrawAt(ms+msNow());
}

//***************************************************************************
// Object
//***************************************************************************

cDisplayItem::cDisplayItem()
   : cThemeItem()
{
   changed = no;
   nextDraw = 0;
   section = 0;
   marquee_active = no;
   backgroundItem = 0;
   visible = yes;
   nextAnimationAt = msNow();
   actLineCount = na;
   lastConditionState = true;

   lastX = 0;
   lastY = 0;
   lastWidth = 0;
   lastHeight = 0;
}

cDisplayItem::~cDisplayItem()
{
}

//***************************************************************************
// Evaluate Color
//***************************************************************************

p_rgba cDisplayItem::evaluateColor(const char* var, p_rgba rgba)
{
   string p = "";

   if (evaluate(p, var) != success)
      memset(rgba, 255, sizeof(t_rgba));   // fallback white
   else
      str2rgba(p.c_str(), rgba);

   tell(3, "evaluated color '%s' to %d/%d/%d/%d (%s)",
        var, rgba[0], rgba[1], rgba[2], rgba[3], p.c_str());

   return rgba;
}

//***************************************************************************
// Evaluate Path
//***************************************************************************

string cDisplayItem::evaluatePath()
{
   string p = "";

   // iterate over path

   for (int i = 0; i < pathCount; i++)
   {
      if (evaluate(p, pathList[i].configured.c_str()) != success)
         continue;

      tell(5, "check path '%s'", p.c_str());

      if (p == "")
      {
         tell(1, "path '%s' empty, skipping",
              pathList[i].configured.c_str());

         continue;
      }

      // append plugin config path

      if (p[0] != '/')
      {
         p = string(GraphTFTSetup.themesPath)
            + string(Thms::theTheme->getDir())
            + "/" + p;

         tell(4, "%d path [%s] converted to '%s'", i,
              pathList[i].configured.c_str(), p.c_str());
      }

      if (fileExists(p.c_str()))
         return p;

      // time for next image?
      //  -> else we don't need to check for range

      if (nextAnimationAt > msNow())
         return pathList[i].last;

      // check for range

      unsigned int s = p.find_last_of('(');
      unsigned int e = p.find_last_of(')');
      int rangeSize = e-s-1;
      int cur;

      if (s != string::npos && e != string::npos && rangeSize >= 3)
      {
         Scan scan(p.substr(s+1, rangeSize).c_str(), no);
         int minNum, maxNum;
         string path = "";

         scan.eat();

         if (!scan.isNum())
            continue;                        // range parsing error

         minNum = scan.lastInt();
         scan.eat();

         if (!scan.isOperator() || *scan.lastIdent() != '-')
            continue;                        // range parsing error

         scan.eat();

         if (!scan.isNum())
            continue;                        // range parsing error

         maxNum = scan.lastInt();

         if (scan.eat() == success)
            continue;                        // range parsing error (waste behind second int)

         // get actual number

         cur = pathList[i].curNum;

         // reset ...

         if (cur < minNum)
         {
            pathList[i].curNum = minNum;
            cur = minNum-1;
         }

         do
         {
            if (++cur > maxNum)
               cur = minNum;

            path = p.substr(0, s) + Str::toStr(cur) + p.substr(e+1);
            tell(2, "Checking for '%s'", path.c_str());

         } while (!fileExists(path.c_str()) && cur != pathList[i].curNum);

         if (fileExists(path.c_str()))
         {
            tell(4, "Animated image '%s'", path.c_str());
            pathList[i].last = path;
            pathList[i].curNum = cur;
            nextAnimationAt = msNow() + _delay;

            return pathList[i].last;
         }
      }
   }

   tell(2, "Image for '%s' with %d elements not found :(", _path.c_str(), pathCount);

   return "";
}

//***************************************************************************
// Replay Mode Value
//***************************************************************************

int cDisplayItem::replayModeValue(ReplayMode rm)
{
   bool play, forward;
   int speed;

   if (!vdrStatus->_replay.control
       || !vdrStatus->_replay.control->GetReplayMode(play, forward, speed))
      return -1;

   switch (rm)
   {
      case rmSpeed:   return speed;
      case rmForward: return forward;
      case rmPlay:    return play;
      default:        return na;
   }

   return na;
}

//***************************************************************************
// Evaluate Condition
//***************************************************************************

int cDisplayItem::evaluateCondition(int recurse)
{
   static Scan* scan = 0;

   int result;
   int state;

   int rightType = catUnknown;
   int leftType = catUnknown;
   int leftInt = 0;
   int rightInt = 0;
   string leftStr = "";
   string rightStr = "";

   char op[100]; *op = 0;
   char logicalOp[100]; *logicalOp = 0;
   string expression;

   if (!recurse || !scan)
   {
      if (_condition.size() <= 0)
         return yes;

      // beim Fehler erst mal 'no' ... ?

      if (evaluate(expression, _condition.c_str()) != success)
         return no;

      tell(3, "evaluating condition '%s' with expression '%s'",
           _condition.c_str(), expression.c_str());

      // ...

      if (scan) delete scan;
      scan = new Scan(expression.c_str());
   }

   // left expression

   scan->eat();

   if (scan->isNum())
   {
      leftInt = scan->lastInt();
      leftType = catInteger;
   }
   else if (scan->isString())
   {
      leftStr = scan->lastString();
      leftType = catString;
   }
   else
   {
      tell(0, "Error: Invalid left '%s' expression in '%s'",
           scan->lastIdent(), expression.c_str());
      return no;
   }

   // operator ?

   if ((state = scan->eat()) == success && scan->isOperator() && !scan->isLogical())
   {
      strcpy(op, scan->lastIdent());

      // right expression

      scan->eat();

      if (scan->isNum())
      {
         rightInt = scan->lastInt();
         rightType = catInteger;
      }
      else if (scan->isString())
      {
         rightStr = scan->lastString();
         rightType = catString;
      }
      else
      {
         tell(0, "Error: Invalid right '%s' expression in '%s'",
              scan->lastIdent(), expression.c_str());
         return no;
      }

      // check the condition

      if (leftType != rightType)
      {
         tell(0, "Error: Argument types of left and right "
              "agrument don't match in (%d/%d) '%s'",
              leftType, rightType, expression.c_str());
         return no;
      }

      if (leftType == catInteger)
         result = condition(leftInt, rightInt, op);
      else
         result = condition(&leftStr, &rightStr, op);

      state = scan->eat();
   }
   else if (leftType == catInteger)
   {
      result = leftInt ? true : false;
   }
   else
   {
      result = leftStr != "" ? true : false;
   }

   // any more expressions in here?

   tell(4, "check for further condition at '%s'", Str::notNull(scan->next()));

   if (state == success)
   {
      tell(4, "further condition found");

      if (!scan->isLogical())
      {
         tell(0, "Error: Invalid logical operator '%s' expression in '%s'",
              scan->lastIdent(), expression.c_str());
         return no;
      }

      strcpy(logicalOp, scan->lastIdent());

      // start a recursion ...

      if (strncmp(logicalOp, "&", 1) == 0)
         result = result && evaluateCondition(yes);
      else if (strncmp(logicalOp, "|", 1) == 0)
         result = result || evaluateCondition(yes);
   }

   tell(3, "condition is '%s'; evaluated condition is '%s'; result is '%s'",
        _condition.c_str(), expression.c_str(), result ? "match" : "don't match");

   return result;
}

//***************************************************************************
// evaluate the condition
//***************************************************************************

int cDisplayItem::condition(int left, int right, const char* op)
{
   tell(4, "evaluate condition '%d' '%s' '%d'", left, op, right);

   if (strcmp(op, ">") == 0)
      return left > right;

   if (strcmp(op, "<") == 0)
      return left < right;

   if (strcmp(op, ">=") == 0)
      return left >= right;

   if (strcmp(op, "<=") == 0)
      return left <= right;

   if (strcmp(op, "=") == 0 || strcmp(op, "==") == 0)
      return left == right;

   if (strcmp(op, "!=") == 0 || strcmp(op, "<>") == 0)
      return left != right;

   tell(0, "Unexpected operator '%s'", op);

   return no;
}

int cDisplayItem::condition(string* left, string* right, const char* op)
{
   tell(4, "evaluate condition '%s' '%s' '%s'",
        left->c_str(), op, right->c_str());

   if (strcmp(op, ">") == 0)
      return *left > *right;

   if (strcmp(op, "<") == 0)
      return *left < *right;

   if (strcmp(op, ">=") == 0)
      return *left >= *right;

   if (strcmp(op, "<=") == 0)
      return *left <= *right;

   if (strcmp(op, "=") == 0 || strcmp(op, "==") == 0)
      return *left == *right;

   if (strcmp(op, "!=") == 0 || strcmp(op, "<>") == 0)
      return *left != *right;

   tell(0, "Unexpected operator '%s'", op);

   return no;
}

//***************************************************************************
// Interface
//***************************************************************************

int cDisplayItem::reset()
{
   if (_scroll)
   {
      marquee_active = yes;
      marquee_left = no;
      marquee_idx = na;
      marquee_count = 0;
      marquee_strip = 0;
      // scheduleDrawIn(0);
   }

   nextAnimationAt = msNow();
   lastWidth = 0;

   return done;
}

int cDisplayItem::draw()
{
   int status = success;
   int cond = true;

   // check condition

   if (!isOfGroup(groupMenu) && !isOfGroup(groupTextList))
   {
      cond = evaluateCondition();

      if (!cond)
      {
         tell(4, "Ignore drawing of '%s' due to condition '%s'",
              nameOf(), _condition.c_str());

         status = ignore;
      }
   }

   // schedule due to the configured delay ..

   if (cond && _delay > 0 && visible && msNow() > nextDraw && !_scroll)
      scheduleDrawIn(_delay);

   // condition state changed force immediate redraw !

   if (lastConditionState != cond)
   {
      tell(4, "Condition '%s' of '%s' [%s] changed from (%d) to (%d), force draw",
           _condition.c_str(), nameOf(),
           Text() != "" ? Text().c_str() : Path().c_str(),
           lastConditionState, cond);

      lastConditionState = cond;
      scheduleForce(msNow() + 10);
   }

   return status;
}

int cDisplayItem::refresh()
{
   tell(6, "timeMs::Now() (%ldms);", msNow());
   tell(6, "nextForce at (%ldms)", nextForce);
   tell(6, "forceDraw '%s', nextDraw (%ldms), isForegroundItem(%d), Foreground(%d)",
        forceDraw ? "yes" : "no", nextDraw, isForegroundItem(), Foreground());

   changed = no;

   // LogDuration ld("cDisplayItem::refresh()");

   // force required ? (volume, animating, osd-message, ...)

   if (nextForce && msNow() >= nextForce)
   {
      forceDraw = yes;
      nextForce = 0;
   }

   // respect the maximal redraw granularity

   if ((nextDraw && (msNow() >= nextDraw-(maxGranularity/2-1)))
       || isForegroundItem() || forceDraw || Foreground())
   {
      nextDraw = 0;
      int res = draw() == success ? 1 : 0;

      changed = res > 0;

      tell(2, "draw '%s', %s", nameOf(), res ? "done" : "skipped due to condition");

      if (res > 0 && logLevel >= 3)
      {
         if (isForegroundItem() || forceDraw || Foreground())
         {
            tell(3, "forceDraw(%d), isForegroundItem(%d), Foreground(%d)",
                 forceDraw,  isForegroundItem(), Foreground());
            tell(3, "'%s' - '%s'", nameOf(),
                 Text().size() ? Text().c_str() : Path().c_str());
         }
      }

      return res;
   }

   return 0;
}

//***************************************************************************
// Painters
//***************************************************************************

int cDisplayItem::drawText(const char* text, int y,
                           int height, int clear, int skipLines)
{
   int width;
   unsigned int viewLength = clen(text);
   unsigned int textLen = clen(text);  // character count (real chars)
   int lineHeight = 0;

   y       = y ? y : Y();
   width   = Width() ? Width() : Thms::theTheme->getWidth() - X();
   height  = height != na? height : Height();
   height  = height != na ? height : Thms::theTheme->getHeight() - y;
   skipLines = skipLines ? skipLines : StartLine();

   if (!height)
      return done;

   // draw background

   if (clear)
      drawBackRect(y, _bg_height ? _bg_height : height);

   if (!textLen || Str::isEmpty(text))
      return done;

   // text width in pixel

   int textWidth = render->textWidthOf(text, _font.c_str(), _size, lineHeight);
   lineHeight = !lineHeight ? 1 :lineHeight;

   // respect max height for line count

   int lines = height / lineHeight > 0 ? height / lineHeight : 1;

   if (_lines > 0)
      lines = std::min(lines, _lines);

   int visibleWidth = width*lines;

   if (textWidth <= 0)
   {
      textWidth = 22 * textLen;

      tell(1, "Info: Can't detect text with of '%s'[%s](%d) witch font %s/%d. Assuming %dpx",
           text, _debug.c_str(), textLen, _font.c_str(), _size, textWidth);
   }

   int charWidth = textWidth / textLen > 0 ? textWidth / textLen : 1;  // at least one pixel :p
   viewLength = visibleWidth / charWidth;  // calc max visible chars

   tell(4, "[%s] drawing text '%s' at %d/%d (%d/%d), '%s'(%d) lines (%d)!",
        _debug.c_str(), text, X(), y, width, height, _font.c_str(), _size, lines);

   tell(3, "[%s] textLen %d, viewLength = %d, textWidth = %dpx, visibleWidth = %dpx, lines = %d, font %s/%d",
        _debug.c_str(), textLen, viewLength, textWidth, visibleWidth, lines, _font.c_str(), _size);

   tell(5, "[%s] scroll is (%d) and marquee_active is (%d) for text '%s'",
        _debug.c_str(), _scroll, marquee_active, text);

   // get line count of the actual text

   actLineCount = render->lineCount(text, _font.c_str(), _size, width);

   // ...

   // exclude item fom scrolling if more than one line displayed,
   // multiline srcolle is prepared but dont work cause of the word warp featute in ImlibRenderer::text(...)
   //  -> to hard to calculate this here ...

   if (!_scroll || textWidth < visibleWidth || lines > 1)
   {
      t_rgba rgba;

      // normal and 'dots' mode

      render->text(text,
                   _font.c_str(), _size, _align,
                   X(), y,
                   evaluateColor(_color.c_str(), rgba),
                   width, height, lines,
                   _dots, skipLines);
   }

   else
   {
      // marquee and ticker mode

      // viewLength -= 3;

      if (msNow() > nextAnimationAt)
      {
         if (_scroll == 1 && marquee_idx + viewLength > textLen)
            marquee_left = yes;

         else if (_scroll == 2 && marquee_idx + viewLength > textLen)
            marquee_idx = na;

         if (marquee_left)
            marquee_idx--;
         else
            marquee_idx++;

         if (marquee_idx == 0)
         {
            marquee_left = no;
            marquee_count++;
         }

         if (_scroll_count && marquee_count > _scroll_count)
         {
            marquee_active = no;
            marquee_idx = 0;
         }

         if (marquee_active)
         {
            if (_delay < 200)
               _delay = 200;

            if (marquee_idx == 0 || marquee_idx > (int)(textLen - viewLength))
               scheduleDrawIn(_delay*3);
            else
               scheduleDrawIn(_delay);

            nextAnimationAt = nextDraw;
         }
      }

      int blen = strlen(text);
      int cs, ps;
      int i = 0;
      t_rgba rgba;

      for (ps = 0; ps < blen; ps += cs)
      {
         i++;
         cs = std::max(mblen(&text[ps], blen-ps), 1);

         if (i >= marquee_idx)
            break;
      }

      tell(3, "drawing text in scroll mode '%s' (%d/%d), nextDraw is %ld; idx is %d/%d",
           text, textLen, viewLength, nextDraw/1000, marquee_idx, ps);

      render->text(text + ps,
                   _font.c_str(), _size, _align,
                   X(), y,
                   evaluateColor(_color.c_str(), rgba),
                   width, height,
                   lines, marquee_active ? no : _dots);
   }

   return done;
}

int cDisplayItem::drawRectangle()
{
   t_rgba rgba;

   render->rectangle(X(), Y(), Width(), Height(),
                     evaluateColor(_color.c_str(), rgba));

   return done;
}

int cDisplayItem::drawBackRect(int y, int height)
{
   int x = _bg_x != na ? _bg_x : X();
   int width = _bg_width > 0 ? _bg_width  : Width();

   y = y ? y : _bg_y != na ? _bg_y : Y();

   if (!height)
      height = _bg_height > 0 ? _bg_height : Height();

   if (!Overlay())
   {
      t_rgba bg_rgba;

      evaluateColor(_bg_color.c_str(), bg_rgba);

      if (haveBackgroundItem())
      {
         string p;

         // fill with part of the backround image

         evaluate(p, backgroundItem->Path().c_str());

         tell(3, "Drawing backround area of '%s' for '%s'",
              p.c_str(), _text.c_str());

         render->imagePart(p.c_str(), x, y, width, height);
      }

      if (bg_rgba[rgbA])
      {
         // fill with solid color, respect alpha channel

         render->rectangle(x, y, width, height, bg_rgba);
      }
   }

   return done;
}

//***************************************************************************
// Get Jpeg Orientation
//***************************************************************************

int getJpegOrientation(const char* file)
{
   int orientation = 1;   // 1 => 'normal'
   ExifData* exifData = exif_data_new_from_file(file);

   if (exifData)
   {
      ExifByteOrder byteOrder = exif_data_get_byte_order(exifData);
      ExifEntry* exifEntry = exif_data_get_entry(exifData, EXIF_TAG_ORIENTATION);

      if (exifEntry)
         orientation = exif_get_short(exifEntry->data, byteOrder);

      exif_data_free(exifData);
   }

   return orientation;
}

int cDisplayItem::drawImage(const char* path, int fit, int aspectRatio, int noBack)
{
   int orientation = 1;    // 1 => 'normal'

   if (!path)             path = _path.c_str();
   if (fit == na)         fit = _fit;
   if (aspectRatio == na) aspectRatio = _aspect_ratio;

   tell(3, "drawing image '%s' at %d/%d (%d/%d); fit = %d; aspectRatio = %d)",
        path, X(), Y(), Width(), Height(), _fit, aspectRatio);

   if (BgWidth() && !noBack)
      drawBackRect();

   if (strcasestr(path, "JPEG") || strcasestr(path, "JPG"))
      orientation = getJpegOrientation(path);

   render->image(path,
                 X(), Y(),
                 Width(), Height(),
                 fit, aspectRatio, orientation);

   return done;
}

//***************************************************************************
// Format String
//***************************************************************************

const char* cDisplayItem::formatString(const char* str, const char* fmt,
                                       char* buffer, int len)
{
   if (Str::isEmpty(fmt) || Str::isEmpty(str))
      return str;

   sprintf(buffer, "%.*s", len, str);

   if (strcasecmp(fmt, "upper") == 0)
      Str::toCase(Str::cUpper, buffer);
   if (strcasecmp(fmt, "lower") == 0)
      Str::toCase(Str::cLower, buffer);

   return buffer;
}

//***************************************************************************
// Format Date Time
//***************************************************************************

const char* cDisplayItem::formatDateTime(time_t theTime, const char* fmt,
                                         char* date, int len, int absolut)
{
   struct tm tim = {0};
   tm* tmp;
   int res;

   *date = 0;

   string format = fmt && *fmt ? fmt :
      (_format.length() ? _format : "%a %d.%m %H:%M");

   // %s seems to be absolut as default ...

   if (absolut && format.find("%s") == string::npos)
   {
      localtime_r(&theTime, &tim);
      theTime += timezone;
   }

   tmp = localtime_r(&theTime, &tim);

   if (!tmp)
   {
      tell(0, "Error: Can't get localtime!");
      return 0;
   }

   res = strftime(date, len, format.c_str(), tmp);

   if (!res)
   {
      tell(0, "Error: Can't convert time, maybe "
           "invalid format string '%s'!", format.c_str());

      return 0;
   }

   if (format.find("%s") != string::npos
       || format.find("%S") != string::npos
       || format.find("%T") != string::npos)
   {
      // refresh in 1 second

      if (!_delay)
         scheduleDrawIn(1000);
   }
   else
   {
      // refresh at next full minute

      scheduleDrawNextFullMinute();
   }

   return date;
}

//***************************************************************************
// Draw Image on Background Coordinates
//***************************************************************************

int cDisplayItem::drawImageOnBack(const char* path, int fit, int height)
{
   if (!path)
      return done;

   int x = _bg_x != na ? _bg_x : X();
   int width = _bg_width > 0 ? _bg_width  : Width();
   int y = _bg_y != na ? _bg_y : Y();

   if (height == na)
      height = _bg_height > 0 ? _bg_height : Height();

   tell(0, "drawing image '%s' at %d/%d (%d/%d)", path, x, y, width, height);

   render->image(path, x, y, width, height, fit);

   return done;
}

int cDisplayItem::drawProgressBar(double current, double total,
                                  string path, int y, int height,
                                  int withFrame, int clear)
{
   t_rgba rgba;
   int xDone;
   char tmp[50];

   int bgX = _bg_x != na ? _bg_x : X();
   int bgWidth = _bg_width ? _bg_width : Width();

   int bgY = y != na ? y : _bg_y != na ? _bg_y : Y();
   int bgHeight = height != na ? height : _bg_height ? _bg_height : Height();

   int red, green, blue, alpha;

   rgba2int(str2rgba(_color.c_str(), rgba), red, green, blue, alpha);

   current = current < 0 ? 0 : current ;
   bgHeight = bgHeight ? bgHeight : Height();
   height = height == na ? Height() : height;
   y = y == na ? Y() : y;

   if (!total) total = 1;
   xDone = (int)((current/total) * (float)Width());

   tell(4, "bar, %f/%f  xDone=%d", current, total, xDone);

   // background

   if (clear)
      drawBackRect(y, height);

   if (_bg_x && withFrame)
      render->rectangle(bgX, bgY,
                        bgWidth, bgHeight,
                        evaluateColor(_bg_color.c_str(), rgba));

   if (path != "")
      render->image(path.c_str(),
                    X(), y,
                    Width(), height,
                    true);
   else // if (_bg_x <= 0)
      render->rectangle(X(), y, Width(), height,
                        evaluateColor(_bg_color.c_str(), rgba));

   // foreground

   if (path != "")
   {
      // with image

      render->rectangle(X() + xDone, y,
                        Width() - xDone, height,
                        evaluateColor(_bg_color.c_str(), rgba));
   }
   else
   {
      // without image

      if (_switch)
      {
         // colorchanging bar

         red = green = 255;
         blue = 0;

         double percent = current / total;

         if (percent < 0.5f)
            red = (int)(255.0f * percent * 2.0f);
         else
            green = (int)(255.0f * (1-percent) * 2.0f);
      }

      render->rectangle(X(), y, xDone, height,
                        int2rgba(red, green, blue, alpha, rgba));
   }

   // draw optional text

   if (_text == "percent")
   {
      int textHeight = _size * 5/3;
      t_rgba rgba;
      sprintf(tmp, "%3.0f%%", current / (total/100));

      render->text(tmp,
                   _font.c_str(), _size, _align,
                   X(),
                   y + (height-textHeight)/2,
                   int2rgba(255-red, 255-green, 255-blue, 0, rgba),
                   bgWidth, height, 1, no);
   }
   else if (_text == "value")
   {
      int textHeight = _size * 5/3;
      t_rgba rgba;

      sprintf(tmp, "%d%s / %d%s",
              (int)current, _unit.c_str(),
              (int)total, _unit.c_str());

      render->text(tmp,
                   _font.c_str(), _size, _align,
                   X(),
                   y + (height-textHeight)/2,
                   int2rgba(255-red, 255-green, 255-blue, 0, rgba),
                   bgWidth, height, 1, no);
   }

   return success;
}

int cDisplayItem::drawPartingLine(string text, int y, int height)
{
   string p;
   int barHeight = BarHeight();

   if (BarHeightUnit() == iuPercent)      // BarHeight in % of row
      barHeight = (int)(((double)height/100) * (double)BarHeight());

   int offset = (height - barHeight) / 2;

   tell(5, "drawing parting line at %d/%d (%d/%d)",
        X(), y+offset, Width(), barHeight);

   if (_path != "" && text != "")
   {
      evaluate(p, _path.c_str());
      render->image(p.c_str(),
                    X(), y+offset,
                    Width(), barHeight,
                    Fit(), AspectRatio());
   }

   if (_path2 != "" && text == "")
   {
      evaluate(p, _path2.c_str());
      render->image(p.c_str(),
                    X(), y+offset,
                    Width(), barHeight,
                    Fit(), AspectRatio());
   }

   if (text != "")
   {
      t_rgba rgba;
      int alignOff = 0;
      int textOff = 0;

      if (AlignV())
         alignOff = (barHeight-(_size * 5/3))/2;

      if (strncmp(text.c_str(), "->", 2) == 0)
         textOff = 2;

      render->text(text.c_str() + textOff,
                   _font.c_str(), _size, _align,
                   X(), y + offset + alignOff,
                   evaluateColor(_color.c_str(), rgba),
                   Width(), barHeight, 1);
   }

   return done;
}

//***************************************************************************
// Item Classes
//***************************************************************************

int cDisplayText::draw()
{
   string p;

   evaluate(p, _text.c_str());

   int vLines = Height() / (_size * 5/3);
   int width = Width() ? Width() : Thms::theTheme->getWidth() - X();

   // trim

   p = trim(p);

   // user scrolling

   if (p != lastText || StartLine() < 0)
   {
      lastText = p;
      setStartLine(0);
   }

   if (StartLine() >= lineCount() - vLines)
      setStartLine(lineCount() - vLines);

   // first ... calc text width and real needed height

   int lineHeight = 0;
   lastHeight = 0;
   lastWidth = render->textWidthOf(p.c_str(), _font.c_str(), _size, lineHeight);
   lastY = Y();
   lastX = X();

   int neededLines = 0;

   if (lastWidth && p.length())
   {
      neededLines = render->lineCount(p.c_str(), _font.c_str(), _size, width);
      lastHeight = neededLines * lineHeight;
      lastHeight = std::min(lastHeight, Height());                    // respect configured max height
   }

   tell(3, "Dimension of '%s' %d/%d now %d/%d; position is %d/%d, need %d lines [%d,%d,%d]",
        _id.c_str(), Width(), Height(),
        lastWidth, lastHeight,
        X(), Y(), neededLines,
        lastWidth, width, lineHeight);

   // second ... check condition

   if (cDisplayItem::draw() != success)
      return fail;

   // third ... draw text

   return drawText(p.c_str(), lastY, lastHeight);
}

int cDisplayRectangle::draw()
{
   if (cDisplayItem::draw() != success)
      return fail;

   return drawRectangle();
}

int cDisplayImage::draw()
{
   if (cDisplayItem::draw() != success)
      return fail;

   string path = evaluatePath().c_str();

   tell(3, "Looking for file '%s'", path.c_str());

   if (!Str::isEmpty(path.c_str()))
   {
      if (_path2 != "")
      {
         string p;
         evaluate(p, _path2.c_str());
         drawImageOnBack(p.c_str(), _fit);
      }

      return drawImage(path.c_str(), na, na, _path2 != "");
   }

   drawBackRect();

   return done;
}

int cDisplayImageFile::draw()
{
   if (cDisplayItem::draw() != success)
      return fail;

   FILE* fp;
   char line[1000+TB]; *line = 0;
   char* c;

   fp = fopen(_path.c_str(), "r");

   if (fp)
   {
      c = fgets(line, 1000, fp);

      if (c) line[strlen(line)] = 0;
      Str::allTrim(line);

      fclose(fp);
   }

   if (!_fit || _aspect_ratio)
      drawBackRect();

   // info

   tell(5, "Looking for file '%s'", line);

   if (!Str::isEmpty(line) && fileExists(line))
   {
      drawImage(line);
   }
   else
   {
      tell(4, "Info: Image '%s' not found, falling back to '%s'",
           line, _path2.c_str());

      drawImage(_path2.c_str());
   }

   return success;
}

//***************************************************************************
// cDisplayImageDir
//***************************************************************************

int cDisplayImageDir::init()
{
   if (cDisplayItem::init() != success)
      return fail;

   images.clear();

   scanDir(_path.c_str());

   tell(0, "Info: Added %ld images of path '%s'", images.size(), _path.c_str());
   current = images.end();

   return success;
}

int cDisplayImageDir::scanDir(const char* path, int level)
{
   const char* extensions = "jpeg:jpg";
   const char* ext;
   DIR* dir;
   struct dirent *entry, *res;
   int nameMax, direntSize;

   // calculate dirent size

   nameMax = pathconf(path, _PC_NAME_MAX);
   nameMax = nameMax > 0 ? nameMax : 256;
   direntSize = offsetof(struct dirent, d_name) + nameMax + TB;

   // open directory

   if (!(dir = opendir(path)))
   {
      tell(1, "Can't open directory '%s', '%s'", path, strerror(errno));
      return done;
   }

   entry = (struct dirent*)malloc(direntSize);

   // iterate ..

   tell(0, "Info: Scanning %sdirectory '%s' for images", level ? "sub-" : "", path);

   while (readdir_r(dir, entry, &res) == 0 && res)
   {
      ImageFile f;

      if (entry->d_type == DT_DIR)
      {
         char* subPath;

         if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

         asprintf(&subPath, "%s/%s", path, entry->d_name);
         scanDir(subPath, level+1);

         free(subPath);
      }

      // check extension

      if ((ext = strrchr(res->d_name, '.')))
         ext++;

      if (Str::isEmpty(ext))
      {
         tell(0, "skipping file '%s' without extension", res->d_name);
         continue;
      }

      if (!strcasestr(extensions, ext))
      {
         tell(0, "skipping file '%s' with extension '%s'", res->d_name, ext);
         continue;
      }

      // fill image infos

      f.path = path + string("/") + res->d_name;
      f.initialized = no;
      f.orientation = 1;    // 1 => 'normal'
      f.landscape = yes;
      f.width = 0;
      f.height = 0;

      images.push_back(f);

      tell(3, "Info: Added '%s'", f.path.c_str());
   }

   free(entry);
   closedir(dir);

   return success;
}

int cDisplayImageDir::getNext(std::vector<ImageFile>::iterator& it, ImageFile*& file)
{
   if (it != images.end())
      it++;

   if (it == images.end())
      it = images.begin();

   if (it == images.end())
      return fail;

   file = &(*it);

   if (!file->initialized)
   {
      file->orientation = getJpegOrientation(file->path.c_str());
      jpegDimensions(file->path.c_str(), file->width, file->height);

      file->landscape = (file->orientation < 5 && file->width < file->height) || (file->orientation >= 5 && file->width > file->height) ? no : yes;
   }

   return success;
}

int cDisplayImageDir::draw()
{
   ImageFile* file;

   if (cDisplayItem::draw() != success)
      return fail;

   if (!_fit || _aspect_ratio)
      drawBackRect();

   if (getNext(current, file) != success)
       return done;

   if (!file->landscape)
   {
      static int n = 0;

      unsigned int width = Width() / 2;
      std::vector<ImageFile>::iterator next = current;

      // hochkant -> show one image at the left side of the screen

      render->image(file->path.c_str(), 0, Y(), width, Height(), yes, yes, _rotate ? file->orientation : 1);

      // now fill the right halfe of the screen

      do
      {
         getNext(next, file);

      } while ((n%2 && file->landscape) || (!(n%2) && !file->landscape));

      n++;

      if (!file->landscape)
      {
         // one 'hochkant' image at the right side of the screen

         render->image(file->path.c_str(), width, Y(), width, Height(), yes, yes, _rotate ? file->orientation : 1);
      }
      else
      {
         const unsigned int offset = 20;
         unsigned int height = (Height()-30) / 2;

         // two 'quer' images on the right side of the screen

         render->image(file->path.c_str(), width, 10, width, height, yes, yes, _rotate ? file->orientation : 1);

         do
         {
            getNext(next, file);

         } while (!file->landscape);

         render->image(file->path.c_str(), width+offset, height+20, width-offset, height, yes, yes, _rotate ? file->orientation : 1);
      }

      current++;
   }
   else
   {
      // quer -> show one image on screen

      drawImage(file->path.c_str());
   }

   current++;

   return success;
}

//***************************************************************************
// cDisplayCalibrationCursor
//***************************************************************************

int cDisplayCalibrationCursor::draw()
{
   if (cDisplayItem::draw() != success)
      return fail;

   t_rgba rgba;
   int width = Width() ? Width() : 20;
   int height = Height() ? Height() : 20;

   if (_path != "")
      render->image(_path.c_str(),
                    vdrStatus->calibration.cursorX - width/2,
                    vdrStatus->calibration.cursorY - height/2,
                    width, height, yes);
   else
      render->rectangle(vdrStatus->calibration.cursorX - width/2,
                        vdrStatus->calibration.cursorY - height/2,
                        width, height,
                        evaluateColor(_color.c_str(), rgba));

   return success;
}

int cDisplayMenuButton::draw()
{
   if (cDisplayItem::draw() != success)
      return fail;

   int index = _item - itemMenuButton;

   if (index >= 0 && index <= 3)
      return drawText(vdrStatus->_menu.buttons[index].c_str(),
                      0, na, no /*clear*/);

   return fail;
}

int cDisplayMenuButtonBackground::draw()
{
   string p;

   if (cDisplayItem::draw() != success)
      return fail;

   int index = _item - itemMenuButtonBackground;

   if (index < 0 || index > 3)
      return fail;

   if (vdrStatus->_menu.buttons[index].length())
   {
      if (_path != "")
      {
         evaluate(p, _path.c_str());
         return drawImage(p.c_str());
      }
   }
   else if (_path2 != "")
   {
      evaluate(p, _path2.c_str());
      return drawImage(p.c_str());
   }

   return done;
}

int cDisplayMessage::draw()
{
   if (cDisplayItem::draw() != success)
      return fail;

   if (vdrStatus->_message != "")
   {
      drawBackRect();

      if (_path != "")
      {
         string p;
         evaluate(p, _path.c_str());
         drawImageOnBack(p.c_str(), _fit);
      }

      tell(3, "draw message '%s'", vdrStatus->_message.c_str());
      drawText(vdrStatus->_message.c_str(), 0, na, no /*clear*/);

      return success;
   }

   return ignore;
}

int cDisplayVolumeMuteSymbol::draw()
{
   static int lastMute = vdrStatus->_mute;
   static uint64_t showUntil = msNow();

   if (cDisplayItem::draw() != success)
      return fail;

   visible = no;

   if (!_permanent && lastMute != vdrStatus->_mute)
   {
      lastMute = vdrStatus->_mute;
      showUntil = msNow() + _delay;
   }

   if (_permanent || msNow() < showUntil)
   {
      string p;

      visible = yes;

      if (!_permanent)
         scheduleForce(showUntil);

      if (vdrStatus->_mute)
      {
         evaluate(p, _path.c_str());
         return drawImage(p.c_str());   // is muted
      }
      else if (_path2 != "")
      {
         evaluate(p, _path2.c_str());
         return drawImage(p.c_str());
      }
   }

   return ignore;
}

int cDisplayVolumebar::draw()
{
   static int lastVolume = vdrStatus->_volume;
   static uint64_t showUntil = msNow();

   if (cDisplayItem::draw() != success)
      return fail;

   visible = no;

   if (!_permanent && lastVolume != vdrStatus->_volume)
   {
      lastVolume = vdrStatus->_volume;
      showUntil = msNow() + _delay;
   }

   if (_permanent || msNow() < showUntil)
   {
      string p;

      if (!_permanent)
         scheduleForce(showUntil);

      visible = yes;

      if (_path2 != "")
      {
         evaluate(p, _path2.c_str());
         drawImageOnBack(p.c_str());
      }

      evaluate(p, _path.c_str());

      return drawProgressBar(vdrStatus->_volume, 255, p,
                             Y(), Height(), no /*withFrame*/, no /*clear*/);
   }

   return ignore;
}

int cDisplayTimebar::draw()
{
   if (cDisplayItem::draw() != success)
      return fail;

   if (!vdrStatus->_presentEvent.isEmpty() && !vdrStatus->_followingEvent.isEmpty())
   {
      string path = evaluatePath().c_str();

      drawProgressBar(time(0) - vdrStatus->_presentEvent.StartTime(),
                      vdrStatus->_followingEvent.StartTime()
                      - vdrStatus->_presentEvent.StartTime(),
                      path);
   }
   else
      drawBackRect();

   if (!_delay)
      scheduleDrawIn(SECONDS(30));

   return done;
}

int cDisplayProgressBar::draw()
{
   if (cDisplayItem::draw() != success)
      return fail;

   string cur;
   string tot;

   if (evaluate(cur, _value.c_str()) == success
       && evaluate(tot, _total.c_str()) == success)
   {
      tell(3, "Progress: '%s'/'%s'   %d/%d",
           cur.c_str(), tot.c_str(),
           atoi(cur.c_str()), atoi(tot.c_str()));

      string path = evaluatePath().c_str();

      return drawProgressBar(atoi(cur.c_str()), atoi(tot.c_str()), path);
   }

   return ignore;
}

int cDisplaySysinfo::draw()
{
   int status = ignore;

   if (cDisplayItem::draw() != success)
      return fail;

   string path = evaluatePath().c_str();

   if (_type.find("cpu") == 0)
   {
      int load = Sysinfo::cpuLoad();

      // if (forceDraw || abs(load - lastCpuLoad) > 2)
      {
         lastCpuLoad = load;

         if (_type == "cpuload")
            status = drawProgressBar(load, 100, path);
         else if (_type == "cpuidle")
            status = drawProgressBar(100-load, 100, path);
      }
   }

   else if (_type.find("mem") == 0)
   {
      unsigned long total, used, free, cached;

      Sysinfo::memInfoMb(total, used, free, cached);

      if (forceDraw || used != lastUsedMem)
      {
         int f = _factor / (1024*1024);  // due to memInfoMb already return MB

         lastUsedMem = used;

         // scale to given factor

         total /= f;
         used  /= f;
         free  /= f;

         if (_type == "memused")
            status = drawProgressBar(used, total, path);
         else if (_type == "memfree")
            status = drawProgressBar(free, total, path);
         else if (_type == "memcached")
            status = drawProgressBar(cached, total, path);
      }
   }

   else if (_type == "disk" && _reference != "")
   {
      unsigned long freeM = 0, usedM = 0;
      char* dir = strdup(_reference.c_str());
      char* c;

      if ((c = strchr(dir, '?')))
      {
         int u;

         for (int i = 0; i < 10; i++)
         {
            *c = '0' + i;

            if (fileExists(dir))
            {
               tell(6, "adding size of '%s'", dir);
               freeM += FreeDiskSpaceMB(dir, &u);
               usedM += u;
            }
         }
      }
      else
      {
         freeM = FreeDiskSpaceMB(dir, (int*)&usedM);
      }

      free(dir);

      // trick, at least 1 due to divide by zero error

      usedM = usedM ? usedM : 1;
      freeM = freeM ? freeM : 1;

      if (forceDraw || usedM != lastUsedDisk)
      {
         int f = _factor / (1024*1024);  // due to FreeDiskSpaceMB return MB

         lastUsedDisk = usedM;

         // scale to given factor

         usedM /= f;
         freeM /= f;

         status = drawProgressBar(usedM, freeM + usedM, path);
      }
   }
   else
   {
      // return without scheduling next draw !

      tell(0, "Ignoring sysinfo item of unexpected type '%s'",
        _type.c_str());

      return fail;
   }

   return status;
}

int cDisplayBackground::draw()
{
   if (cDisplayItem::draw() != success)
      return fail;

   // #TODO - implement a force of all other items?

   if (_path != "")
      return cDisplayImage::draw();

   return drawRectangle();
}

int cDisplayTextList::draw()
{
   cGraphTFTDisplay::cTextList* list = 0;

   if (!_lines)
      _lines = 1;

   string p;
   int visible = 0;
   int y = 0;
   int lineHeight = _size * 5/3;
   int maxRows = Height() / (lineHeight * _lines);

   cDisplayItem::draw();

   // music or timer list ?

   if (strstr(_text.c_str(), "actTimers") || strstr(_text.c_str(), "actRunning") || strstr(_text.c_str(), "actPending"))
      list = &vdrStatus->_timers;
   else
      list = &vdrStatus->_music;

   // first count visible rows

   lastHeight = 0;
   list->reset();

   // first clear background, event if path is set (image may be transparent!)

   drawBackRect();

   // draw optional background image

   if (_path != "")
      drawImageOnBack(_path.c_str(), yes);

   while (visible < maxRows && list->isValid())
   {
      if (evaluateCondition())
         visible++;

      list->inc();
   }

   if (visible == 0)
      return success;

   tell(3, "draw list with %d visible items, top pos %d", visible, Y());

   // draw list

   list->reset();

   for (int r = 0; r < visible && list->isValid(); list->inc())
   {
      if (!evaluateCondition())
         continue;

      evaluate(p, _text.c_str());

      y = Y() + lineHeight * r * _lines;

      tell(4, "TextList, row %d '%s' at (%d/%d) with (%d) lines [%s]",
           r, p.c_str(), X(), y, _lines, _text.c_str());

      drawText(p.c_str(), y, lineHeight * _lines, no);

      lastHeight += lineHeight * _lines;
      r++;
   }

   return success;
}

int cDisplayMenuSelected::draw()
{
   if (cDisplayItem::draw() != success)
      return fail;

   selectedItem = this;

   return done;
}

int cDisplayMenuColumnSelected::draw()
{
   if (cDisplayItem::draw() != success)
      return fail;

   selectedItem = this;

   return done;
}

int cDisplayMenuEventColumnSelected::draw()
{
   if (cDisplayItem::draw() != success)
      return fail;

   selectedItem = this;

   return done;
}

//***************************************************************************
// Menu - old style
//***************************************************************************

int cDisplayMenu::draw()
{
   if (cDisplayItem::draw() != success)
      return fail;

   if (!selectedItem)
      return fail;

   int total;
   int tabPercent[cGraphTFTService::MaxTabs] = {0, 0, 0, 0, 0, 0};
   int afterSelect = 0;
   int lineHeight = _size * 5/3;
   int lineHeightSelect = selectedItem->Size() * 5/3;
   int count = ((Height()-lineHeightSelect) / lineHeight) + 1;
   int step = vdrStatus->_menu.currentRow - vdrStatus->_menu.currentRowLast;     // step since last refresh

   vdrStatus->_menu.visibleRows = count;
   vdrStatus->_menu.lineHeight = lineHeight;
   vdrStatus->_menu.lineHeightSelected = lineHeightSelect;

   if (vdrStatus->_menu.topRow < 0)
      vdrStatus->_menu.topRow = std::max(0, vdrStatus->_menu.currentRow - count/2);   // initial
   else if (vdrStatus->_menu.currentRow == vdrStatus->_menu.topRow-1)
      vdrStatus->_menu.topRow = vdrStatus->_menu.currentRow;                     // up
   else if (vdrStatus->_menu.currentRow == vdrStatus->_menu.topRow+count)
      vdrStatus->_menu.topRow++;                                                 // down
   else if (vdrStatus->_menu.currentRow < vdrStatus->_menu.topRow
            || vdrStatus->_menu.currentRow > vdrStatus->_menu.topRow+count)
      vdrStatus->_menu.topRow += step;                                           // page up / page down

   if (vdrStatus->_menu.topRow > (int)vdrStatus->_menu.items.size()-count)
      vdrStatus->_menu.topRow = vdrStatus->_menu.items.size()-count;

   if (vdrStatus->_menu.topRow < 0)
      vdrStatus->_menu.topRow = 0;

   vdrStatus->_menu.currentRowLast = vdrStatus->_menu.currentRow;

   // calculate column width

   total = vdrStatus->_menu.charInTabs[0] + vdrStatus->_menu.charInTabs[1] + vdrStatus->_menu.charInTabs[2]
      + vdrStatus->_menu.charInTabs[3] + vdrStatus->_menu.charInTabs[4] + vdrStatus->_menu.charInTabs[5];

   if (!total)
      return done;

   for (int i = 0; vdrStatus->_menu.charInTabs[i] != 0; i++)
      tabPercent[i] = (int)((100L / (double)total) * (double)vdrStatus->_menu.charInTabs[i]);

   tell(4, "Debug: tabs set to - %d:%d:%d:%d:%d",
        tabPercent[0], tabPercent[1], tabPercent[2],
        tabPercent[3], tabPercent[4]);

   // loop over visible rows ...

   for (int i = vdrStatus->_menu.topRow; i < std::min((int)vdrStatus->_menu.items.size(),
         vdrStatus->_menu.topRow + count); ++i)
   {
      cDisplayItem* p = i == vdrStatus->_menu.currentRow ? selectedItem : this;
      int y = p->Y() + ((i - vdrStatus->_menu.topRow) * lineHeight);

      if (i == vdrStatus->_menu.currentRow)
      {
         afterSelect = lineHeightSelect - lineHeight;

         // draw the selected backround

         if (selectedItem->Focus() != "")
         {
            string p;

            evaluate(p, selectedItem->Focus().c_str());
            render->image(p.c_str(), X(),
                          Y() + (i - vdrStatus->_menu.topRow) * lineHeight,
                          Width(), Height());
         }

         // draw the columns

         int x = selectedItem->X();

         for (int t = 0; t < vdrStatus->_menu.items[i].tabCount; ++t)
         {
            if (vdrStatus->_menu.items[i].tabs[t] != "")
            {
               int width = (Width() * tabPercent[t]) / 100;
               t_rgba rgba;

               render->text(vdrStatus->_menu.items[i].tabs[t].c_str(),
                            selectedItem->Font().c_str(), selectedItem->Size(), selectedItem->Align(),
                            x,
                            Y() + ((i - vdrStatus->_menu.topRow) * lineHeight),
                            evaluateColor(selectedItem->Color().c_str(), rgba),
                            width,
                            lineHeightSelect, 1);

               x += width;
            }
         }

         if (selectedItem->Path() != "")
            render->image(selectedItem->Path().c_str(),
                          X(), Y() + (i - vdrStatus->_menu.topRow) * lineHeight,
                          Width(), Height());

         if (p->StaticPicture())
         {
            // for the selected item we optionaly draw a additional picture (ImageMap)
            // as image name use the text after the number in the last column ...

            string path = Thms::theTheme->getPathFromImageMap(vdrStatus->_menu.items[i].tabs[vdrStatus->_menu.items[i].tabCount-1].c_str());

            if (path != "")
            {
               if (p->StaticX() && p->StaticY())
                  render->image(path.c_str(), p->StaticX(), p->StaticY(),
                                p->StaticWidth(), p->StaticHeight(), yes, yes);
               else if (p->StaticX())
                  render->image(path.c_str(),
                                p->StaticX(),
                                Y() + (i - vdrStatus->_menu.topRow) * lineHeight
                                - ((p->StaticHeight() - lineHeightSelect) /2),
                                p->StaticWidth(), p->StaticHeight(), yes, yes);
            }
         }
      }

      else
      {
         if (_path != "")
            render->image(_path.c_str(), X(), y + afterSelect, Width(), Height());

         int x = X();

         for (int t = 0; t < vdrStatus->_menu.items[i].tabCount; ++t)
         {
            if (vdrStatus->_menu.items[i].tabs[t] != "")
            {
               int width = (Width() * tabPercent[t]) / 100;
               t_rgba rgba;

               render->text(vdrStatus->_menu.items[i].tabs[t].c_str(),           // test,
                            _font.c_str(), _size, _align,                        // font, font-size, align
                            x, y + afterSelect,
                            evaluateColor(_color.c_str(), rgba),
                            width, lineHeight, 1);

               x += width;
            }
         }
      }
   }

   return done;
}

//***************************************************************************
// Menu Column - new 'column' style
//***************************************************************************

int cDisplayMenuColumn::draw()
{
   if (cDisplayItem::draw() != success)
      return fail;

   cGraphTFTDisplay::MenuInfo* _menu = &vdrStatus->_menu;
   int count;
   int lineHeight;
   int lineHeightSelect;

   int step = _menu->currentRow - _menu->currentRowLast;  // step since last refresh
   int afterSelect = 0;

   if (!_menu->items.size())
      return done;

   // calc height and row count ...

   lineHeight = Height() ? Height() : Size() * 5/3;

   if (selectedItem)
      lineHeightSelect = selectedItem->Height() ? selectedItem->Height() : selectedItem->Size() * 5/3;
   else
      lineHeightSelect = lineHeight;

   count = ((_menu_height-lineHeightSelect) / lineHeight) + 1;

   // tell(0, "_menu->items.size(%d), _menu->topRow(%d), _menu->currentRow(%d),"
   //      "_menu->currentRowLast(%d), count(%d)",
   //     _menu->items.size(), _menu->topRow, _menu->currentRow, _menu->currentRowLast, count);

   _menu->visibleRows = count;
   _menu->lineHeight = lineHeight;
   _menu->lineHeightSelected = lineHeightSelect;

   if (_menu->topRow < 0)
      _menu->topRow = std::max(0, _menu->currentRow - count/2);     // initial
   else if (_menu->currentRow == _menu->topRow-1)
      _menu->topRow = _menu->currentRow;                       // up
   else if (_menu->currentRow == _menu->topRow+count)
      _menu->topRow++;                                         // down
   else if (_menu->currentRow < _menu->topRow
            || _menu->currentRow > _menu->topRow+count)
      _menu->topRow += step;                                   // page up / page down

   if (_menu->topRow > (int)_menu->items.size()-count)
      _menu->topRow = _menu->items.size()-count;

   if (_menu->topRow < _menu->currentRow-count)
      _menu->topRow = _menu->currentRow-count;

   if (_menu->topRow < 0)
      _menu->topRow = 0;

   if (step)
      marquee_active = no;

   _menu->currentRowLast = _menu->currentRow;

   // paint the visible rows for this column

   for (int i = _menu->topRow; i < std::min((int)_menu->items.size(), _menu->topRow + count); ++i)
   {
      cDisplayItem* p = this;

      if (i == _menu->currentRow)
      {
         afterSelect = lineHeightSelect - lineHeight;

         if (!selectedItem || selectedItem->Number() == na)
            continue;

         p = selectedItem;
      }

      int y = p->MenuY() + ((i - _menu->topRow) * lineHeight) + (p == selectedItem ? 0 : afterSelect);
      _menu->drawingRow = i;  // the row

      tell(4, "colcount of row (%d) is (%d)", i, _menu->items[i].tabCount);

      if (p->Number() > vdrStatus->_menu.items[i].tabCount)
      {
         tell(0, "Warning: Skipping column number %d, only (%d) columns for row (%d) send by vdr",
              Number(), vdrStatus->_menu.items[vdrStatus->_menu.drawingRow].tabCount, vdrStatus->_menu.drawingRow);

         continue;
      }

      if (!p->evaluateCondition())
         continue;

      // calc pos an width ...

      if (p->X() == na)
         p->setX(_menu->items[i].nextX);

      if (!p->Width())
         p->setWidth(Thms::theTheme->getWidth()-p->X());

      _menu->items[i].nextX = p->X() + p->Width() + p->Spacing();

      if (i == _menu->currentRow)
      {
         if (p->ImageMap() && Number())
         {
            // for the selected item we optionaly draw a additional picture (ImageMap)
            // as image name use the text of the current column ...

            string path = Thms::theTheme->getPathFromImageMap(_menu->items[i].tabs[p->Number()-1].c_str());

            if (path != "" && p->StaticX())
               render->image(path.c_str(), p->StaticX(),
                             p->StaticY() ? p->StaticY() : y - ((p->StaticHeight() - lineHeightSelect) /2),
                             p->StaticWidth(), p->StaticHeight(), yes, yes);
         }

         if (p->StaticPicture() && Number())
         {
            // Image with static position for this column

            if (_menu->items[i].tabs[p->Number()-1].c_str()[0] && p->StaticX())
               render->image(p->channelLogoPath(_menu->items[i].tabs[p->Number()-1].c_str(), Format().c_str()).c_str(),
                             p->StaticX(), p->StaticY() ? p->StaticY() : y, p->StaticWidth(), p->StaticHeight(), yes, yes);
         }

         if (p->StaticText() && Number())
         {
            t_rgba rgba;

            // Text with static position for this column

            if (_menu->items[i].tabs[p->Number()-1].c_str()[0])
            {
               render->text(_menu->items[i].tabs[p->Number()-1].c_str(),   // text
                            p->Font().c_str(), p->Size(), p->Align(),      // font, font-size, align
                            p->StaticX(),                                  // x-pos
                            p->StaticY(),                                  // y-pos
                            evaluateColor(p->Color().c_str(), rgba),       // color
                            p->StaticWidth(),                              // width
                            lineHeight, 1);                                // height, line-count
            }
         }
      }

      if (p->Width() == na)
         continue;

      if (_menu->items[i].type == itPartingLine)
      {
         cDisplayItem* partingLine = section->getItemByKind(itemPartingLine);

         if (partingLine)
            partingLine->drawPartingLine(_menu->items[i].tabs[0], y, lineHeight);

         continue;
      }

      // draw image

      if (p->Focus() != "")
      {
         string f;
         evaluate(f, p->Focus().c_str());
         render->image(f.c_str(),
                       p->X(), y,
                       p->Width(), lineHeight);
      }

      if (p->Type() == "progress")
      {
         int current = 0;
         int n = 0;
         int barHeight = p->BarHeight();

         if (p->BarHeightUnit() == iuPercent)      // BarHeight in % of row
            barHeight = (int)(((double)lineHeight/100) * (double)p->BarHeight());

         // calc progress

         while (_menu->items[i].tabs[p->Number()-1][n])
         {
            if (_menu->items[i].tabs[p->Number()-1][n] == '|')
               current++;
            n++;
         }

         // draw column's progress-bar

         tell(5, "progress [%d] of (%d%%) at position y = %d [%s]",
              i, current, y, _menu->items[i].tabs[p->Number()-1].c_str());

         if (current && p->Path() != "")
         {
            string path = evaluatePath().c_str();

            p->drawProgressBar(current, 8, path, y + (lineHeight-barHeight)/2, barHeight);
         }
      }

      else if (p->Type() == "image")
      {
         string path;

         if (p->Path() != "")
         {
            if (p->Path() == "{imageMap}")
               path = Thms::theTheme->getPathFromImageMap(_menu->items[i].tabs[p->Number()-1].c_str());
            else
               path = p->evaluatePath();
         }
         else
            path = p->channelLogoPath(_menu->items[i].tabs[p->Number()-1].c_str(), Format().c_str());

         // draw column's image

         if (path.length())
         {
            int barHeight = p->BarHeight();

            if (p->BarHeightUnit() == iuPercent)      // BarHeight in % of row
               barHeight = (int)(((double)lineHeight/100) * (double)p->BarHeight());

            int offset = (lineHeight - barHeight) / 2;

            render->image(path.c_str(), p->X(), y+offset,
                          p->Width(), barHeight,
                          p->Fit(), p->AspectRatio());
         }
      }

      else
      {
         // draw column's text

         string text;

         if (p->Text() != "")
         {
            if (evaluate(text, p->Text().c_str()) != success)
               text = "";
         }
         else
            text = _menu->items[i].tabs[p->Number()-1].c_str();

         tell(5, "draw '%s'", text.c_str());

         p->drawText(text.c_str(), y, lineHeight, no);
      }
   }

   selectedItem = 0;

   return done;
}

//***************************************************************************
// Menu Event Column
//***************************************************************************

int cDisplayMenuEventColumn::draw()
{
   // static int selectedOffset = 0;

   if (cDisplayItem::draw() != success)
      return fail;

   if (!vdrStatus->_eventsReady)
      return fail;

   cGraphTFTDisplay::MenuInfo* _menu = &vdrStatus->_menu;

   string path;
   int count;
   int lineHeight;
   int lineHeightSelect = 0;

   int step = _menu->currentRow - _menu->currentRowLast;  // step since last refresh
   int afterSelect = 0;

   // calc height and row count ...

   lineHeight = Height() ? Height() : Size() * 5/3;

   if (selectedItem)
      lineHeightSelect = selectedItem->Height() ? selectedItem->Height() : selectedItem->Size() * 5/3;
   else
      lineHeightSelect = lineHeight; // assume normal height this time

   count = ((_menu_height-lineHeightSelect) / lineHeight) + 1;

   _menu->visibleRows = count;
   _menu->lineHeight = lineHeight;
   _menu->lineHeightSelected = lineHeightSelect;

   if (_menu->topRow < 0)
      _menu->topRow = std::max(0, _menu->currentRow - count/2);     // initial
   else if (_menu->currentRow == _menu->topRow-1)
      _menu->topRow = _menu->currentRow;                       // up
   else if (_menu->currentRow == _menu->topRow+count)
      _menu->topRow++;                                         // down
   else if (_menu->currentRow < _menu->topRow
            || _menu->currentRow > _menu->topRow+count)
      _menu->topRow += step;                                   // page up / page down

   if (_menu->topRow > (int)_menu->items.size()-count)
      _menu->topRow = _menu->items.size()-count;

   if (_menu->topRow < _menu->currentRow-count)
      _menu->topRow = _menu->currentRow-count;

   if (_menu->topRow < 0)
      _menu->topRow = 0;

   if (step)
      marquee_active = no;

   _menu->currentRowLast = _menu->currentRow;

   // paint the visible rows for this column

   for (int i = _menu->topRow; vdrStatus->_eventsReady && i < std::min((int)_menu->items.size(), _menu->topRow + count); ++i)
   {
      if (i == na)
         tell(0, "XXXXXXXXXXXXXXXXXXXXXXXXX");

      _menu->drawingRow = i;  // the row

      cDisplayItem* p = i == _menu->currentRow ? selectedItem : this;

      if (!p)
         continue;

      int y = p->MenuY() + ((i - _menu->topRow) * lineHeight) + afterSelect;

      tell(4, "colcount of row (%d) is (%d)", i, _menu->items[i].tabCount);

      if (i == _menu->currentRow)
         afterSelect = lineHeightSelect - lineHeight;

      if (_menu->items[i].type == itPartingLine)
      {
         cDisplayItem* partingLine = section->getItemByKind(itemPartingLine);

         // nur für die erste Spalte zeichnen, partingLine
         // soll alle Spalten abdecken!

         if (partingLine && Number() == 0)
            partingLine->drawPartingLine(_menu->items[i].tabs[0], y, lineHeight);

         continue;
      }

      // don't check condition for itPartingLine

      if (!p->evaluateCondition())
         continue;

      // draw focus image

      if (i == _menu->currentRow && p->Focus() != "")
      {
         string f;
         evaluate(f, p->Focus().c_str());
         render->image(f.c_str(),
                       p->X(), y, //  + selectedOffset,
                       p->Width(), lineHeight);
      }

      if (p->Type() == "progress")
      {
         int barHeight = p->BarHeight();
         string value;
         int current;
         string path = "";

         lookupVariable("rowEventProgress", value);

         current = atoi(value.c_str());

         if (p->BarHeightUnit() == iuPercent)      // BarHeight in % of row
            barHeight = (int)(((double)lineHeight/100) * (double)p->BarHeight());

         // draw column's progress-bar

         tell(5, "progress [%d] of (%d%%)", i, current);

         if (current && p->Path() != "")
         {
            string path = evaluatePath().c_str();
            p->drawProgressBar(current, 100, path, y + (lineHeight-barHeight)/2, barHeight);
         }
      }

      else if (p->Type() == "image")
      {
         path = p->evaluatePath();

         // draw column's image

         if (path.length())
         {
            int barHeight = p->BarHeight();

            if (p->BarHeightUnit() == iuPercent)      // BarHeight in % of row
               barHeight = (int)(((double)lineHeight/100) * (double)p->BarHeight());

            int offset = (lineHeight - barHeight) / 2;

            render->image(path.c_str(), p->X(), y+offset,
                          p->Width(), barHeight,
                          p->Fit(), p->AspectRatio());
         }
      }

      else
      {
         // draw column's text

         string text;
         int textHeight = p->Size() * 5/3;
         textHeight -= textHeight/10;              // 10% weniger

         if (evaluate(text, p->Text().c_str()) != success)
               text = "";

         tell(1, "--> draw '%s'", text.c_str());

         if (!p->Line())
            p->drawText(text.c_str(),
                        y + (p->AlignV() ? (lineHeight-textHeight)/2 : 0),
                        lineHeight, no);
         else
            p->drawText(text.c_str(),
                        y + ((p->Line()-1)*textHeight),
                        textHeight, no);
      }
   }

   selectedItem = 0;

   return done;
}

//***************************************************************************
// Spectrum Analyzer
//***************************************************************************

int cDisplaySpectrumAnalyzer::draw()
{
   if (cDisplayItem::draw() != success)
      return fail;

   if (cPluginManager::CallFirstService(SPAN_GET_BAR_HEIGHTS_ID, 0))
   {
      // tell(0, "draw SpectrumAnalyzer II");

      cSpanService::Span_GetBarHeights_v1_0 GetBarHeights;

      int bandsSA = 20;
      int falloffSA = 8;

      unsigned int* barHeights = new unsigned int[bandsSA];
      unsigned int* barHeightsLeftChannel = new unsigned int[bandsSA];
      unsigned int* barHeightsRightChannel = new unsigned int[bandsSA];
      unsigned int* barPeaksBothChannels = new unsigned int[bandsSA];
      unsigned int* barPeaksLeftChannel = new unsigned int[bandsSA];
      unsigned int* barPeaksRightChannel = new unsigned int[bandsSA];
      unsigned int volumeLeftChannel;
      unsigned int volumeRightChannel;
      unsigned int volumeBothChannels;

      GetBarHeights.bands = bandsSA;
      GetBarHeights.barHeights = barHeights;
      GetBarHeights.barHeightsLeftChannel = barHeightsLeftChannel;
      GetBarHeights.barHeightsRightChannel = barHeightsRightChannel;
      GetBarHeights.volumeLeftChannel = &volumeLeftChannel;
      GetBarHeights.volumeRightChannel = &volumeRightChannel;
      GetBarHeights.volumeBothChannels = &volumeBothChannels;

      GetBarHeights.name = "graphtft";
      GetBarHeights.falloff = falloffSA;
      GetBarHeights.barPeaksBothChannels = barPeaksBothChannels;
      GetBarHeights.barPeaksLeftChannel = barPeaksLeftChannel;
      GetBarHeights.barPeaksRightChannel = barPeaksRightChannel;

      if (cPluginManager::CallFirstService(SPAN_GET_BAR_HEIGHTS_ID, &GetBarHeights))
      {
         int i;
         t_rgba rgba;
         int barWidth = Width() / (2 * bandsSA);
         int width = barWidth * 2 * bandsSA;

         tell(4, "width = %d; barWidth = %d; ", width, barWidth);

         if (_path != "")
            render->image(_path.c_str(),
                          X(), Y(), width, Height(), true);
         else
            render->rectangle(X(), Y(),
                              width, Height(),
                              evaluateColor(_color.c_str(), rgba));

         for (i = 0; i < bandsSA; i++)
         {
            int2rgba(0, 0, 0, 255, rgba);

            render->rectangle(X() + 2 * barWidth * i,
                              Y(),
                              barWidth,
                              Height() - (barHeightsLeftChannel[i] * Height() / 100),
                              rgba);


            render->rectangle(X() + 2 * barWidth * i + barWidth,
                              Y(),
                              barWidth,
                              Height() - (barHeightsRightChannel[i] * Height() / 100),
                              rgba);

            // the peak

            //                height = barPeaksBothChannels[i] * item->Height() / 100;

            //                if (height > 0)
            //                {
            //                   render->rectangle(item->X() + barWidth*2*i+ barWidth + 1,
            //                                      item->Y(),
            //                                      item->X() + barWidth*2*i + barWidth+ barWidth + 1,
            //                                      height,
            //                                      item->Red(), item->Green(), item->Blue(),
            //                                      item->Transparent());
            //                }
         }
      }

      delete[] barHeights;
      delete[] barHeightsLeftChannel;
      delete[] barHeightsRightChannel;
      delete[] barPeaksBothChannels;
      delete[] barPeaksLeftChannel;
      delete[] barPeaksRightChannel;
   }

   return done;
}
