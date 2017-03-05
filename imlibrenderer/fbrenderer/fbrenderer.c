/**
 *  GraphTFT plugin for the Video Disk Recorder
 *
 *  fbrenderer.c - A plugin for the Video Disk Recorder
 *
 *  (c) 2004 Lars Tegeler, Sascha Volkenandt
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 * $Id: fbrenderer.c,v 1.4 2012/09/27 13:07:12 wendel Exp $
 *
 **/

#include <sys/mman.h>
#include <sys/ioctl.h>

#include <fbrenderer.h>

#include <libavcodec/avcodec.h>

#include <common.h>
#include <setup.h>

static unsigned char* frame_buffer;
static struct fb_var_screeninfo fb_vinfo;

typedef unsigned char UINT8;

//***************************************************************************
// Object
//***************************************************************************

FbRenderer::FbRenderer(int x, int y, int width, int height,
                       string cfgPath, int utf, string thmPath)
   : ImlibRenderer(x, y, width, height, cfgPath, utf, thmPath)
{
   fb_dev_name = 0;
   initialized = no;
}

FbRenderer::~FbRenderer()
{
	deinit();
}

//***************************************************************************
//
//***************************************************************************

void FbRenderer::deinit()
{
   if (!initialized)
      return;

   fb_orig_vinfo.xoffset = fb_vinfo.xoffset;
   fb_orig_vinfo.yoffset = fb_vinfo.yoffset;

   if (ioctl(fb_dev_fd, FBIOPUT_VSCREENINFO, &fb_orig_vinfo))
      tell(4, "Can't reset original fb_var_screeninfo: %s", strerror(errno));

   ::close(fb_dev_fd);

   if (frame_buffer)
      munmap(frame_buffer, fb_size);

   initialized = no;
}

//***************************************************************************
//
//***************************************************************************

#ifndef AV_PIX_FMT_RGB32
#  define AV_PIX_FMT_RGB32  PIX_FMT_RGB32
#  define AV_PIX_FMT_RGB24  PIX_FMT_RGB24 
#  define AV_PIX_FMT_RGB565 PIX_FMT_RGB565 
#endif

int FbRenderer::init(int lazy)
{
	asprintf(&fb_dev_name, "%s", devname);

	// open framebuffer

   tell(4 , "Using framebuffer device '%s'", fb_dev_name);

	if ((fb_dev_fd = open(fb_dev_name, O_RDWR)) == -1)
   {
		tell(0, "Opening framebuffer device '%s' faild, error was '%s'",
           fb_dev_name, strerror(errno));
		return fail;
	}

	// read VScreen info from fb

   if (ioctl(fb_dev_fd, FBIOGET_VSCREENINFO, &fb_vinfo))
   {
		tell(0, "Can't get VSCREENINFO, %s", strerror(errno));
		return fail;
	}

	// Save VScreen info and try to set virtual image

	fb_orig_vinfo = fb_vinfo;
	fb_vinfo.xres_virtual = fb_vinfo.xres;
	fb_vinfo.yres_virtual = fb_vinfo.yres;

	fb_vinfo.xoffset = 0;
	fb_vinfo.yoffset = 0;

   // fb_vinfo.bits_per_pixel = 32;

	// write VScreen info

	if (ioctl(fb_dev_fd, FBIOPUT_VSCREENINFO, &fb_vinfo))
		tell(0, "Can't put VSCREENINFO, %s", strerror(errno));

	// read VScreen info from fb (again) this will be the 'accepted' resolutions from the fb

	if (ioctl(fb_dev_fd, FBIOGET_VSCREENINFO, &fb_vinfo))
   {
		tell(0, "Can't get VSCREENINFO, %s", strerror(errno));
      return fail;
	}

	// read VScreen info from fb

	if (ioctl(fb_dev_fd, FBIOGET_FSCREENINFO, &fb_finfo))
   {
		tell(0, "Can't get FSCREENINFO, %s", strerror(errno));
      return fail;
	}

   tell(0, "fb settings are (%d/%d) with a color depth of (%d)",
        fb_vinfo.xres, fb_vinfo.yres, fb_vinfo.bits_per_pixel);

   dspWidth = fb_vinfo.xres;
   dspHeight = fb_vinfo.yres;

	y_offset = fb_vinfo.yoffset;
	fb_line_len = fb_finfo.line_length;
	fb_size = fb_finfo.smem_len;
	frame_buffer = 0;

	switch (fb_vinfo.bits_per_pixel)
        {
		case 32: tell(4, "FB using 32 bit depth"); fb_type = AV_PIX_FMT_RGB32;  break;
		case 24: tell(4, "FB using 24 bit depth"); fb_type = AV_PIX_FMT_RGB24;  break;
		case 16: tell(4, "FB using 16 bit depth"); fb_type = AV_PIX_FMT_RGB565; break;
		default: tell(4, "FB color depth not supported -> %i bits per pixel",
                    fb_vinfo.bits_per_pixel);
	}

   if ((frame_buffer = (unsigned char*)mmap(0, fb_size, PROT_READ | PROT_WRITE, MAP_SHARED, fb_dev_fd, 0)) == (unsigned char*)-1)
   {
		tell(0, "FB Can't mmap %s: %s", fb_dev_name, strerror(errno));
		return fail;
	}

	initialized = yes;

   ImlibRenderer::init(lazy);

   return success;
}

//***************************************************************************
//
//***************************************************************************

#ifndef use_asm

void FbRenderer::fbdev_draw_32(unsigned char* frame, int force)
{
	memcpy(frame_buffer, frame, 4*fb_vinfo.yres*fb_vinfo.xres);
}

#else

/* FIXME evil hack */

static void fbdev32(unsigned char * frame)
{
  __asm__ __volatile__(

	"		pushl	%%esi				\n\t"
	"		pushl	%%edi				\n\t"
	"		pushl	%%eax				\n\t"
	"		pushl	%%ecx				\n\t"

	"		movl	fb_vinfo,%%eax		  \n\t"  // Height
	"		movl	fb_vinfo+4,%%ecx	  \n\t"  // width
	"		imul	%%eax,%%ecx         \n\t"  // mul
	"		movl	frame_buffer,%%edi  \n\t"  // fbdev mmap'd buffer
	"		movl	8(%%ebp), %%esi     \n\t"  // Imlib2 buffer (frame)
	"		rep	movsl               \n\t"  // move all longs at a time (4 bytes)

	"		popl	%%ecx				\n\t"
	"		popl	%%eax				\n\t"
	"		popl	%%edi				\n\t"
	"		popl	%%esi				\n\t"

	:/*no output*/:/*no input*/:"memory","cc");
}

void FbRenderer::fbdev_draw_32(unsigned char* frame, int force)
{
   fbdev32(frame);
}

#endif

//***************************************************************************
//
//***************************************************************************

#ifndef use_asm

void FbRenderer::fbdev_draw_24(unsigned char* frame, int force)
{
  unsigned int i,a,b,c,x, out_offset = 0, in_offset = 0;

  x = fb_vinfo.xres*4;

  for (i = 0; i < fb_vinfo.yres; ++i)
  {
     for (a=0, b=0, c=0; a < fb_vinfo.xres; ++a, b+=3, c+=4)
     {
        frame_buffer[out_offset + b +0] = frame[in_offset + c +0];
        frame_buffer[out_offset + b +1] = frame[in_offset + c +1];
        frame_buffer[out_offset + b +2] = frame[in_offset + c +2];
     }

     out_offset += fb_line_len;
     in_offset  += x;
  }
}

#else

/* FIXME evil hack */

static void fbdev24(unsigned char* frame)
{
  __asm__ __volatile__(

	"		pushl	%%esi				\n\t"
	"		pushl	%%edi				\n\t"
	"		pushl	%%eax				\n\t"
	"		pushl	%%ebx				\n\t"
	"		pushl	%%ecx				\n\t"
	"		pushl	%%edx				\n\t"

	"		movl	fb_vinfo,%%eax	    \n\t"  // fbdev mmap'd buffer
	"		movl	fb_vinfo+4,%%edx	 \n\t"  // fbdev mmap'd buffer
	"		imul	%%eax,%%edx			 \n\t"  // fbdev mmap'd buffer
	"		movl	8(%%ebp), %%esi    \n\t"  // Imlib2 buffer (frame)
	"		movl	frame_buffer,%%edi \n\t"  // fbdev mmap'd buffer

	"	.lop:	                      \n\t"
   "     leal	3,%%ecx            \n\t"  // fbdev mmap'd buffer
	"		rep	movsb              \n\t"  // move 3 bytes at a time
	"		inc	%%esi              \n\t"  // increment one byte, bypass Alpha
	"		dec	%%edx              \n\t"  // dec counter
	"		jnz .lop                 \n\t"  // loop :)

	"		popl	%%edx				\n\t"
	"		popl	%%ecx				\n\t"
	"		popl	%%ebx				\n\t"
	"		popl	%%eax				\n\t"
	"		popl	%%edi				\n\t"
	"		popl	%%esi				\n\t"

	:/*no output*/:/*no input*/:"memory","cc");
}

void FbRenderer::fbdev_draw_24(unsigned char* frame, int force)
{
   fbdev24(frame);
}

#endif

//***************************************************************************
// fbdev draw 16
//***************************************************************************

void FbRenderer::fbdev_draw_16(unsigned char* frame, int force)
{
   static unsigned short* tmp = 0;
   static unsigned int size = fb_vinfo.yres * fb_vinfo.xres;
   static unsigned short* fb = (unsigned short*)frame_buffer;

   if (!tmp)
      tmp = (unsigned short*)calloc(sizeof(unsigned short), size);

   LogDuration ld("FbRenderer::fbdev_draw_16()");

   unsigned char B, G, R;
   unsigned int x, y;
   unsigned short v;
   unsigned int out_offset = 0, in_offset = 0;

   for (y = 0; y < fb_vinfo.yres; y++)
   {
      for (x = 0; x < fb_vinfo.xres; x++, out_offset++, in_offset+=4)
      {
         R = (frame[in_offset + 2] >> 3) & 0x1f;
         G = (frame[in_offset + 1] >> 2) & 0x3f;
         B = (frame[in_offset + 0] >> 3) & 0x1f;

         v = ((G << 5) | B)  |  ((R << 3) | (G >> 3)) << 8;

         if (force || tmp[out_offset] != v)
         {
            tmp[out_offset] = v;
            fb[out_offset] = v;
         }
      }
   }
}

//***************************************************************************
// Refresh
//***************************************************************************

void FbRenderer::refresh(int force)
{
   LogDuration ld("FbRenderer::refresh()");

   // refresh

   ImlibRenderer::refresh(force);

   // copy to buffer

   imlib_context_set_image(*pImageToDisplay);

   if (GraphTFTSetup.flipOSD)
   {
      imlib_image_flip_vertical();
      imlib_image_flip_horizontal();
   }

   UINT8* dataptr = (UINT8*)imlib_image_get_data_for_reading_only();

   tell(4, "copy image with a depth of (%d) to framebuffer",
        fb_vinfo.bits_per_pixel);

   switch (fb_vinfo.bits_per_pixel)
   {
      case 16 : fbdev_draw_16(dataptr, force); break;
      case 24 : fbdev_draw_24(dataptr, force); break;
      case 32 : fbdev_draw_32(dataptr, force); break;

      default : tell(0, "fbdevout.c: color depth not supported "
                     "-> %i bits per pixel", fb_vinfo.bits_per_pixel);
   }

#ifdef PVRFB

	struct ivtvfb_ioctl_dma_host_to_ivtv_args prep;
	prep.source = frame_buffer;
	prep.dest_offset = 0;
	prep.count = width * height * 4;
	ioctl(fb_dev_fd, IVTVFB_IOCTL_PREP_FRAME, &prep);

#endif
}

void FbRenderer::clear()
{
	ImlibRenderer::clear();
}
