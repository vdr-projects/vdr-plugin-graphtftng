/**
 *  GraphTFT plugin for the Video Disk Recorder 
 * 
 *  fbrenderer.h - A plugin for the Video Disk Recorder
 *
 *  (c) 2004 Lars Tegeler, Sascha Volkenandt  
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 * $Id: fbrenderer.h,v 1.4 2012/09/27 13:07:12 wendel Exp $
 *
 **/

//The most part of this Code is from the MMS V.2 Project:

#ifndef __GTFT_FBRENDERER_HPP
#define __GTFT_FBRENDERER_HPP

#include <linux/fb.h>

#include <imlibrenderer.h>

//***************************************************************************
// 
//***************************************************************************

class FbRenderer : public ImlibRenderer
{

   public:	

		FbRenderer(int x, int y, int width, int height, 
                 string cfgPath, int utf, string thmPath);
		~FbRenderer();

		int init(int lazy);
		void deinit();

		void refresh(int force = no);
		void clear();

   private:

		void fbdev_draw_32(unsigned char* frame, int force);
		void fbdev_draw_24(unsigned char* frame, int force);
		void fbdev_draw_16(unsigned char* frame, int force);

      // data

		char* fb_dev_name;
		int initialized;
		Imlib_Image _resized;

		int fb_dev_fd;
		int fb_type;
		size_t fb_size;
		int fb_line_len;
		int y_offset;

		struct fb_var_screeninfo fb_orig_vinfo;
		struct fb_fix_screeninfo fb_finfo;
};

#endif // __GTFT_FBRENDERER_H
