//***************************************************************************
// Group VDR/GraphTFT
// File renderer.c
// Date 31.10.06
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
// (c) 2006-2008 Jörg Wendel
//--------------------------------------------------------------------------
// Class Renderer
//***************************************************************************

#include <string.h>

#include "renderer.h"

//***************************************************************************
// Object
//***************************************************************************

Renderer::Renderer(int x, int y, int width, int height, 
                   string cfgPath, int utf, string thmPath)
{ 
   confPath = cfgPath;

   xOffset = x;
   yOffset = y;
   themeWidth = width;
   themeHeight = height;
   utf8 = utf;
   themePath = thmPath;

   dspWidth = width;
   dspHeight = height;
   xBorder = 0;
   yBorder = 0;
   devname = 0;
}

Renderer::~Renderer() 
{
   free(devname);
}

//***************************************************************************
// Set Device Name
//***************************************************************************

void Renderer::setDevName(const char* _devname)
{
   if (!Str::isEmpty(_devname))
   {
      free(devname);
      devname = strdup(_devname);
      tell(0, "Set display to '%s'", devname);
   }
}

//***************************************************************************
// Set Properties
//***************************************************************************

void Renderer::setProperties(int x, int y, int width, int height, 
                             int utf, string thmPath)
{
   xOffset = x;
   yOffset = y;
   themeWidth = width;
   themeHeight = height;
   utf8 = utf;
   themePath = thmPath;
}

//***************************************************************************
// Set Border
//***************************************************************************

void Renderer::setBorder(int widthBorder, int heightBorder)
{
   xBorder = widthBorder;
   yBorder = widthBorder;
}
