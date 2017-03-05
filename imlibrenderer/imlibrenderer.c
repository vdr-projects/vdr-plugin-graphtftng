/**
 *  GraphTFT plugin for the Video Disk Recorder 
 * 
 *  imlibrenderer.c - A plugin for the Video Disk Recorder
 *
 *  (c) 2004      Lars Tegeler, Sascha Volkenandt  
 *  (c) 2006-2013 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 * $Id: imlibrenderer.c,v 1.10 2012/09/27 13:07:11 wendel Exp $
 *
 **/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <locale.h>

#include <string>
#include <vector>
#include <sstream>

#include <string>
#include <utility>
#include <iostream>

#include <jpeglib.h>

#include "imlibrenderer.h"
#include "theme.h"
#include "common.h"

using std::string;
using std::cout;
using std::endl;

//***************************************************************************
// Object
//***************************************************************************

ImlibRenderer::ImlibRenderer(int x, int y, int width, int height, 
                             string cfgPath, int utf, string thmPath)
   : Renderer(x, y, width, height, cfgPath, utf, thmPath)
{
//   Imlib_Context ctx = imlib_context_new();
//   imlib_context_push(ctx);

   _render_image = 0;
   _cur_image = 0;
   pImageToDisplay = 0;
}

ImlibRenderer::~ImlibRenderer()
{
   deinit();
}

//***************************************************************************
// init / deinit
//***************************************************************************

int ImlibRenderer::init(int lazy)
{
   if (_render_image || _cur_image)
      deinit();

   // new image

   imlib_set_color_usage(256);

   _cur_image = imlib_create_image(themeWidth, themeHeight);
   _render_image = imlib_create_image(dspWidth, dspHeight);
   imlib_context_set_image(_cur_image);

   // cache

   imlib_set_cache_size(16 * 1024 * 1024);
   imlib_set_font_cache_size(4 * 1024 * 1024);

   return success;
}

void ImlibRenderer::deinit()
{
   pImageToDisplay = 0;

   if (_render_image)
   {
      imlib_context_set_image(_render_image);
      imlib_free_image();
      _render_image = 0;
   }

   if (_cur_image)
   {
      imlib_context_set_image(_cur_image);
      imlib_free_image();
      _cur_image = 0;
   }
}

//***************************************************************************
// 
//***************************************************************************

void ImlibRenderer::flushCache()
{
   // flush the font cache

   imlib_flush_font_cache();

   // and the image cache

   imlib_set_cache_size(0);
   imlib_set_cache_size(16 * 1024 * 1024);
}

//***************************************************************************
// Set Font Path
//***************************************************************************

void ImlibRenderer::setFontPath(string fntPath)
{
   int e, s;
   e = s = 0;
   string p;
   int count;
   char* path;
   const char** list;

   // first clear imlibs font path

   list = (const char**)imlib_list_font_path(&count);

   for (int i = 0; i < count; i++)
   {
      tell(3, "Info: Removing '%s' from font path", list[i]);
      imlib_remove_path_from_font_path(list[i]);
   }

   imlib_list_font_path(&count);

   // add all configured font paths

   tell(0, "Info: Font path configured to '%s'", fntPath.c_str());
   
   do
   {
      e = fntPath.find(':', s);
      p = fntPath.substr(s, e == na ? fntPath.length()-s : e-s);

      if (p[0] != '/')
      {
         // make path relative to the themes directory

         asprintf(&path, "%s/themes/%s/%s", 
                  confPath.c_str(), themePath.c_str(), p.c_str());
      }
      else
      {
         asprintf(&path, "%s", p.c_str());
      }

      tell(0, "Info: Adding font path '%s'", path);

      if (*path && fileExists(path))
         imlib_add_path_to_font_path(path);
      else
         tell(0, "Info: Font path '%s' not found, ignoring", path);
      
      free(path);
      s = e+1;

   } while (s > 0);
   
   
   // at least add the default path

   asprintf(&path, "%s/graphtftng/fonts/", confPath.c_str());
   tell(0, "Info: Adding font path '%s'", path);
   imlib_add_path_to_font_path(path);
   free(path);
}

//***************************************************************************
// Refresh
//***************************************************************************

void ImlibRenderer::refresh(int force)
{
   LogDuration ld("ImlibRenderer::refresh()");

   if (!_cur_image)
      return ;

   // resize only if needed !

   if (xOffset || yOffset || xBorder || yBorder 
       || themeWidth != dspWidth || themeHeight != dspHeight)
   {
      tell(2, "scale image from (%d/%d) to (%d/%d)", 
        themeWidth, themeHeight, 
        dspWidth - xOffset - (2 * xBorder), 
        dspHeight - yOffset - (2 * yBorder));
      
      imlib_context_set_image(_render_image);

      imlib_blend_image_onto_image(_cur_image, 0,
                                   0, 0, themeWidth, themeHeight,
                                   xOffset + xBorder, yOffset + yBorder,
                                   dspWidth - xOffset - (2 * xBorder), 
                                   dspHeight - yOffset - (2 * yBorder));

      pImageToDisplay = &_render_image;
   }
   else
   {
      pImageToDisplay = &_cur_image;
   }
}

void ImlibRenderer::clear()
{
   if (!_cur_image)
      return ;

	// clear the current image

	imlib_context_set_image(_cur_image);
	imlib_free_image();
	_cur_image = imlib_create_image(themeWidth, themeHeight);
	imlib_context_set_image(_cur_image);
}

//***************************************************************************
// Image
//***************************************************************************

void ImlibRenderer::image(const char* fname, 
                          int x, int y, 
                          int width, int height, 
                          bool fit, bool aspectRatio, 
                          int orientation)
{
   Imlib_Image new_image;
   int imgWidth = 0;
   int imgHeight = 0;
   std::ostringstream path;
   int areaWidth = width;
   int areaHeight = height;
   Imlib_Load_Error err;
   int rotate = na;
   int hflip = no;

   if (!_cur_image)
      return ;

   if (x == na) x = 0;
   if (y == na) y = 0;

   if (fname[0] == '/')
      path << fname;
   else
      path << confPath << "/themes/" << themePath << "/" << fname; 

   if (!fileExists(path.str().c_str()))
   { 
      tell(0, "Image '%s' not found", path.str().c_str());
      return ;
   }

   new_image = imlib_load_image_with_error_return(path.str().c_str(), &err);

   if (!new_image)
   { 
      tell(0, "The image '%s' could not be loaded, error was '%s'", 
           path.str().c_str(), strerror(err));

      return ;
   }

   imlib_context_set_image(new_image);

   switch (orientation)
   {
      case 0: rotate = na; hflip = no;   break;  // 0: unknon
      case 1: rotate = na; hflip = no;   break;  // 1: okay
      case 2: rotate = na; hflip = yes;  break;  // 2: gespiegelt (not implemented yet)
      case 3: rotate = 2;  hflip = no;   break;  // 3: auf dem Kopf
      case 4: rotate = 2;  hflip = yes;  break;  // 4: auf dem Kopf (und gespiegelt -> not implemented yet)
      case 5: rotate = 1;  hflip = yes;  break;  // 5: 90° links (und gespiegelt -> not implemented yet)
      case 6: rotate = 1;  hflip = no;   break;  // 6: 90° links
      case 7: rotate = 3;  hflip = yes;  break;  // 7: 90° rechts (und gespiegelt -> not implemented yet)
      case 8: rotate = 3;  hflip = no;   break;  // 8: 90° rechts
   }

   if (rotate != na)
      imlib_image_orientate(rotate);

   if (hflip)
      imlib_image_flip_horizontal();

   imgWidth = imlib_image_get_width();
   imgHeight = imlib_image_get_height();

   if (strstr(fname, "chg_"))
      imlib_image_set_changes_on_disk();

   imlib_context_set_image(_cur_image);

   if (fit)
   {
      if (aspectRatio)
      {
         double ratio = (double)imgWidth / (double)imgHeight;

         if ((double)width/(double)imgWidth < (double)height/(double)imgHeight)
         {
            height = (int)((double)width / ratio);
            y += (areaHeight-height) / 2;
         }
         else
         {
            width = (int)((double)height * ratio);
            x += (areaWidth-width) / 2;
         }
      }

      imlib_blend_image_onto_image(new_image, 0, 0, 0, 
                                   imgWidth, imgHeight, x, y, 
                                   width, height);
   }
   else
   {
      imlib_blend_image_onto_image(new_image, 0, 0, 0, 
                                   imgWidth, imgHeight, x, y, 
                                   imgWidth, imgHeight);
   }
  
   imlib_context_set_image(new_image);
   imlib_free_image();
   imlib_context_set_image(_cur_image);
}

//***************************************************************************
// Image Part
//***************************************************************************

void ImlibRenderer::imagePart(const char* fname, int x, int y, 
                              int width, int height)
{
   Imlib_Image new_image;
   std::ostringstream path;
   Imlib_Load_Error err;

   if (!_cur_image)
      return ;

   if (x == na) x = 0;
   if (y == na) y = 0;

   if (fname[0] == '/')
      path << fname;
   else
      path << confPath << "/themes/" << themePath << "/" << fname; 

   if (!fileExists(path.str().c_str()))
   { 
      tell(0, "Image '%s' not found", path.str().c_str());
      return ;
   }

   // new_image = imlib_load_image(path.str().c_str());

   new_image = imlib_load_image_with_error_return(path.str().c_str(), &err);

   if (!new_image)
   {
      tell(0, "The image '%s' could not be loaded, error was '%s'", 
           path.str().c_str(), strerror(err));

      return ;
   }

   imlib_context_set_image(_cur_image);

   imlib_blend_image_onto_image(new_image, 0, 
                                x, y, width, height,
                                x, y, width, height);

   imlib_context_set_image(new_image);
   imlib_free_image();
   imlib_context_set_image(_cur_image);
}

//***************************************************************************
// Text Width Of
//***************************************************************************

int ImlibRenderer::textWidthOf(const char* text, const char* fontName,
                               int fontSize, int& height)
{
   Imlib_Font font;
   int width = 20;
   char* fontNameSize = 0;

   fontSize = fontSize ? fontSize : 24;
   height = fontSize * (5/3);

   if (!_cur_image)
      return 0;

   if (!fontName || !text || !strlen(text))
      return 0;

   asprintf(&fontNameSize, "%s/%d", fontName, fontSize);
   font = imlib_load_font(fontNameSize);
   free(fontNameSize);

   if (font)
   {
      imlib_context_set_font(font);
		imlib_context_set_image(_cur_image);
      imlib_get_text_size(text, &width, &height);
      imlib_free_font();
   }  
	else
		tell(1, "The font '%s' could not be loaded.", fontName);

   return width;
}

//***************************************************************************
// Char Width Of
//***************************************************************************

int ImlibRenderer::charWidthOf(const char* fontName, int fontSize)
{
   Imlib_Font font;
   int width = 20;
   int height = 20;
   char* fontNameSize = 0;

   if (!_cur_image)
      return 30;

   const char* text = "We need here something like a "
      "representive sentence WITH SOME UPPER CASE LETTERS";

   if (!fontName)
   {
      imlib_get_text_size(text, &width, &height);
      return width / strlen(text);
   }

   asprintf(&fontNameSize, "%s/%d", fontName, fontSize);
   font = imlib_load_font(fontNameSize);
   free(fontNameSize);

   if (font)
   {
      imlib_context_set_font(font);
		imlib_context_set_image(_cur_image);
      imlib_get_text_size(text, &width, &height);
      imlib_free_font();

      width /= clen(text);
   }  
	else
		tell(1, "The font '%s' could not be loaded.", fontName);

   return width;
}

//***************************************************************************
// Line Count
//***************************************************************************

int ImlibRenderer::lineCount(const char* text, const char* font_name, 
                             int size, int width)
{
   int count;
   int lineHeight, textWidth, dummy;
   string line;
   string tmp = text;
   Imlib_Font font;
   int currentWidth;
   int blankWidth, dotsWidth;
   string::size_type pos = 0;
   char* fontNameSize = 0;

   if (!_cur_image)
      return 1;

   // load font

   asprintf(&fontNameSize, "%s/%d", font_name, size);
   font = imlib_load_font(fontNameSize);
   free(fontNameSize);

   if (width <= 0)
      width = themeWidth;          // ;)

   imlib_context_set_font(font);
   imlib_context_set_image(_cur_image);

   imlib_get_text_size(tmp.c_str(), &textWidth, &lineHeight);
   imlib_get_text_size(" ", &blankWidth, &dummy);
   imlib_get_text_size("...", &dotsWidth, &dummy);

   for (count = 1; pos < tmp.length(); count++)
   {
      string token;
      line = "";

      currentWidth = 0;

      // fill next line ...

      while (pos < tmp.length())
      {
         int tokenWidth;
         int lf = no;

         string::size_type pA = tmp.find_first_of(" \n", pos);

         if (pA != string::npos && tmp[pA] == '\n')
         {
            lf = yes;
            tmp[pA] = ' ';
         }

         token = tmp.substr(pos, pA - pos);

         if (token == "")
         {
            line += " ";
            pos++;

            if (lf)
               break;
            else
               continue;
         }

         imlib_get_text_size(token.c_str(), &tokenWidth, &dummy);
         
         if (currentWidth + (currentWidth ? blankWidth : 0) + tokenWidth > width)
         {
            // passt nicht mehr ganz rein

            if (!line.length())
            {
               // alleinstehendes Wort -> noch zur Zeile rechnen

               pos += token.length();
            }

            break;
         }
         else
         {
            // passt noch rein

            line = line + token;
            imlib_get_text_size(line.c_str(), &currentWidth, &dummy);
            pos += token.length();

            if (lf)
               break;
         }
      } 
   }

   imlib_free_font();

   return count-1;
}

//***************************************************************************
// Draw Text
//***************************************************************************

int ImlibRenderer::text(const char* text, 
                        const char* font_name, int size, 
                        int align, int x, int y, 
                        p_rgba rgba, // int r, int g, int b, 
                        int width, int height, 
                        int lines, int dots, int skipLines)
{
   string line;
   string tmp;
   int lineHeight, textWidth, dummy;
   int blankWidth;
   int dotsWidth;
   string::size_type pos;
   int currentWidth;
   Imlib_Font font;
   char* fontNameSize = 0;

   if (x == na) x = 0;
   if (y == na) y = 0;

   if (!_cur_image)
      return 0;

   if (Str::isEmpty(text))
      return 0;
 
   if (utf8)
   {
      const int maxBuf = 10000;
      char out[maxBuf+TB];

      if (toUTF8(out, maxBuf, text) == success)
         tmp = out;
      else
         tmp = text;
   }
   else
      tmp = text;

   // load font

   asprintf(&fontNameSize, "%s/%d", font_name, size);
   font = imlib_load_font(fontNameSize);
   free(fontNameSize);

   if (width <= 0)
      width = themeWidth;          // ;)

   imlib_context_set_font(font);
   imlib_context_set_image(_cur_image);
   imlib_context_set_color(rgba[rgbR], rgba[rgbG], rgba[rgbB], 255);

   imlib_get_text_size(tmp.c_str(), &textWidth, &lineHeight);
   imlib_get_text_size(" ", &blankWidth, &dummy);
   imlib_get_text_size("...", &dotsWidth, &dummy);

   if (!lines)
      lines = height / lineHeight;

   if (lines <= 0)
      lines = 1;

   pos = 0;
   int tl = 1;

   for (int l = 1; l <= lines && pos < tmp.length(); l++, tl++)
   {
      int yPos = y + lineHeight * (l-1);
      string token;
      line = "";

      currentWidth = 0;

      while (pos < tmp.length())
      {
         int tokenWidth;
         int lf = no;

         string::size_type pA = tmp.find_first_of(" \n", pos);

         if (pA != string::npos && tmp[pA] == '\n')
         {
            lf = yes;
            tmp[pA] = ' ';
         }

         token = tmp.substr(pos, pA - pos);

         if (token == "")
         {
            line += " ";
            pos++;

            if (lf)
               break;
            else
               continue;
         }

         imlib_get_text_size(token.c_str(), &tokenWidth, &dummy);
         
         if (currentWidth + (currentWidth ? blankWidth : 0) + tokenWidth > width)
         {
            // passt nicht mehr ganz rein

            if (!line.length() || l == lines)
            {
               // alleinstehendes Wort oder letzte Zeile,
               // daher Abgeschnitten anzeigen

               unsigned int i = 0;

               while (currentWidth + (dots ? dotsWidth : 0) < width && i <= token.length())
               {
                  line += token.substr(i++, 1);
                  imlib_get_text_size(line.c_str(), &currentWidth, &dummy);
               }

               // einer zuviel !

               line = line.substr(0, line.length()-1);

               if (dots)
               {
                  if (currentWidth + dotsWidth > width)
                     line = line.substr(0, line.length()-1);
                  
                  line += "...";
               }

               pos += token.length();
            }

            break;
         }
         else
         {
            // passt noch rein

            // currentWidth += blankWidth + tokenWidth;  // nicht genau
            // -> so ist's besser

            line = line + token;
            imlib_get_text_size(line.c_str(), &currentWidth, &dummy);
            pos += token.length();

            if (lf)
               break;
         }
      } 

      if (skipLines && tl <= skipLines)
      {
         l--;
         continue;
      }

      if (align != 0)
         imlib_get_text_size(line.c_str(), &textWidth, &lineHeight);

      if (align == 0)                // left
         imlib_text_draw(x, yPos, line.c_str());
      else if (align == 1)           // center
         imlib_text_draw(x + (width - textWidth) / 2, yPos, line.c_str());
      else                           // right
         imlib_text_draw(x + width - textWidth -2, yPos, line.c_str());
   }

   imlib_free_font();

   return 0;
}

//***************************************************************************
// Draw Rectangle
//***************************************************************************

void ImlibRenderer::rectangle(int x, int y, 
                              int width, int height, 
                              p_rgba rgba)  // int r, int g, int b, int alpha) 
{
   if (x == na) x = 0;
   if (y == na) y = 0;

   if (!_cur_image)
      return ;

   imlib_context_set_image(_cur_image);
   imlib_context_set_color(rgba[rgbR], rgba[rgbG], rgba[rgbB], rgba[rgbA]);
   imlib_image_fill_rectangle(x, y, width, height);
}

//***************************************************************************
// Dump Image To File
//***************************************************************************

void ImlibRenderer::dumpImage2File(const char* fname, int dumpWidth, 
                                   int dumpHeight, const char* aPath)
{
   std::ostringstream path; 
   const char* format = 0;

   if (!_cur_image)
      return ;

   if (aPath && *aPath)
      path << aPath;
   else if (*fname != '.' && *fname != '/')
      path << "/tmp";
   
   if (strchr(fname, '.'))
      format = strchr(fname, '.') + 1;

	// check if local directory structure exist, else create it.

   if (!fileExists(path.str().c_str())) 
      mkdir(path.str().c_str(), 0777);

	// get width and heigt

   imlib_context_set_image(_cur_image);
	int width = imlib_image_get_width();
	int height = imlib_image_get_height();

   // create image

  	Imlib_Image new_image = imlib_create_image(dumpWidth, dumpHeight);

	imlib_context_set_image(new_image);
	imlib_blend_image_onto_image(_cur_image, 0,
                                0, 0, width, height, 
                                0, 0, dumpWidth, dumpHeight);

   tell(1, "DUMP: From (%d/%d) to (%d/%d) '%s'",
        width, height, dumpWidth, dumpHeight, fname);
   
	// save the image

	imlib_image_set_format(format && *format ? format : "png");
	path << "/" << fname;
	imlib_save_image(path.str().c_str());

	imlib_free_image();
	imlib_context_set_image(_cur_image);
}

//***************************************************************************
// To JPEG
//***************************************************************************

#ifndef WITH_TCPCOM

long ImlibRenderer::toJpeg(unsigned char*& buffer, int quality)
{
   return 0;
}

#else

long ImlibRenderer::toJpeg(unsigned char*& buffer, int quality)
{
   struct jpeg_compress_struct cinfo = { 0 };
   struct jpeg_error_mgr jerr;
   DATA32* ptr;
   DATA8* buf;
   unsigned long size;

   if (!_cur_image)
      return 0;

   buffer = 0;
   size = 0;
   
   cinfo.err = jpeg_std_error(&jerr);

   jpeg_create_compress(&cinfo);
   jpeg_mem_dest(&cinfo, &buffer, &size);

   cinfo.image_width = imlib_image_get_width();
   cinfo.image_height = imlib_image_get_height();
   cinfo.input_components = 3;
   cinfo.in_color_space = JCS_RGB;

   jpeg_set_defaults(&cinfo);
   jpeg_set_quality(&cinfo, quality, TRUE);
   jpeg_start_compress(&cinfo, TRUE);

   // get data pointer

   if (!(ptr = imlib_image_get_data_for_reading_only()))
      return 0;
   
   // allocate a small buffer to convert image data */

   buf = (DATA8*)malloc(imlib_image_get_width() * 3 * sizeof(DATA8));

   while (cinfo.next_scanline < cinfo.image_height) 
   {
      // convert scanline from ARGB to RGB packed

      for (int j = 0, i = 0; i < imlib_image_get_width(); i++)
      {
         buf[j++] = ((*ptr) >> 16) & 0xff;
         buf[j++] = ((*ptr) >>  8) & 0xff;
         buf[j++] = ((*ptr))       & 0xff;

         ptr++;
      }

      // write scanline

      jpeg_write_scanlines(&cinfo, (JSAMPROW*)(&buf), 1);
   }
   
   free(buf);
   jpeg_finish_compress(&cinfo);
   jpeg_destroy_compress(&cinfo);
   
   return size;
}
#endif
