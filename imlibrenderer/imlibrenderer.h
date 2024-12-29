/**
 *  GraphTFT plugin for the Video Disk Recorder 
 * 
 *  imlibrenderer.h
 *
 *  (c) 2004 Lars Tegeler, Sascha Volkenandt
 *  (c) 2006-2014 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 **/

#ifndef __GTFT_IMLIBRENDERER_HPP
#define __GTFT_IMLIBRENDERER_HPP

#include <Imlib2.h>
#include "renderer.h"

#ifndef WITH_X
#  define X_DISPLAY_MISSING
#endif

#undef Status

//***************************************************************************
// Class ImlibRenderer
//***************************************************************************

class ImlibRenderer : public Renderer
{
	public:

		ImlibRenderer(int x, int y, int width, int height, string cfgPath, int utf, string thmPath);
		virtual ~ImlibRenderer();

      int init(int lazy);
      void deinit();
      void flushCache();
      void setFontPath(string fntPath);

      long toJpeg(unsigned char*& buffer, int quality);

		virtual void refresh(int force = no);
		virtual void clear();

      int textWidthOf(const char* text, const char* fontName, int fontSize, int& height);
      int charWidthOf(const char* fontName = 0, int fontSize = 0);

		void image(const char* fname, int x, int y, 
                 int coverwidth, int coverheight, 
                 bool fit = no, bool aspectRatio = no, 
                 int orientation = 1);
      void imagePart(const char* fname, int x, int y, int width, int height);

		int text(const char* text, const char* font_name, int size, int align, 
               int x, int y,
               p_rgba rgba, // int r, int g, int b, 
               int width, int height,int lines, int dots = 0, int skipLines = 0);

      int lineCount(const char* text, const char* font_name, int size, int width);

      void rectangle(int x, int y, int width, int height, p_rgba rgba);

		void dumpImage2File(const char* fname, int dumpWidth, int dumpHeight, const char* aPath = 0);

   protected:

		Imlib_Image _cur_image;               // the image you're working on
		Imlib_Image _render_image;            // the image buffer for scaling
		Imlib_Image* pImageToDisplay;         // pointer to the image for displaying
};

//***************************************************************************
#endif // __GTFT_IMLIBRENDERER_H
