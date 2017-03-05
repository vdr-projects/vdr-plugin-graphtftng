/**
 *  GraphTFT plugin for the Video Disk Recorder 
 * 
 *  renderer.h - A plugin for the Video Disk Recorder
 *
 *  (c) 2005-2013 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 **/

#ifndef __GTFT_RENDERER_H
#define __GTFT_RENDERER_H

#include <string>

#include <common.h>

using std::string;

//***************************************************************************
// used as base class of the renderes
//***************************************************************************

class Renderer
{
   public:

      Renderer(int x, int y, int width, int height, 
               string cfgPath, int utf, string thmPath);
      virtual ~Renderer();

      virtual void setDevName(const char* _devname);
      virtual void setProperties(int x, int y, 
                                 int width, int height, 
                                 int utf, string thmPath);
      virtual void setBorder(int widthBorder, 
                              int heightBorder);
      virtual void setDisplaySize(int width, int height) { }
      virtual void setFontPath(string fntPath) = 0;
      virtual int init(int lazy) = 0;
      virtual void flushCache() {};
      virtual void deinit() = 0;

      virtual long toJpeg(unsigned char*& buffer, int quality) = 0;

      virtual void refresh(int force = no) = 0;
      virtual void refreshArea(int x, int y, int width, int height) {}
      virtual void clear() = 0;

      virtual int textWidthOf(const char* text, const char* fontName, int fontSize, int& height) = 0;
      virtual int charWidthOf(const char* fontName = 0, 
                              int fontSize = 0) = 0;
      virtual void image(const char* fname, int x, int y, 
                         int width, int height, 
                         bool fit = no, bool aspectRatio = no, 
                         int orientation = 1) = 0;

      virtual void imagePart(const char* fname, int x, int y, 
                             int width, int height) = 0;

      virtual int text(const char *text, const char *font_name, 
                       int size, int align, int x, int y, 
                       p_rgba rgba, // int r, int g, int b, 
                       int width, int height,
                       int lines, int dots = 0, int skipLines = 0) = 0;

      virtual int lineCount(const char* text, const char* font_name, 
                            int size, int width) = 0;

      virtual void rectangle(int x, int y, int width, int height, 
                             p_rgba rgba) = 0;
                         // int r, int g, int b, int alpha) = 0;

      virtual void dumpImage2File(const char* fname, 
                                  int dumpWidth, int dumpHeight, 
                                  const char* aPath = 0) = 0;

      virtual int xPending()                      { return done; }
      virtual int attach(const char* disp = 0)    { return done; }
      virtual int detach()                        { return done; }

   protected:

      string confPath;
      string themePath;
      int utf8;
      int xOffset;
      int yOffset;
      int themeWidth;
      int themeHeight;
      int xBorder;
      int yBorder;

      int dspWidth;
      int dspHeight;
      char* devname;
};

//***************************************************************************
#endif // __GTFT_RENDERER_H
