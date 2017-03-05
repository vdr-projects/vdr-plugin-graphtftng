//***************************************************************************
// Group VDR/GraphTFT
// File dmyrenderer.c
// Date 31.10.06 - Jörg Wendel
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
//--------------------------------------------------------------------------
// Class DummyRenderer
//***************************************************************************

#ifndef __GTFT_DMYRENDERER_HPP__
#define __GTFT_DMYRENDERER_HPP__

#include "imlibrenderer.h"

class DummyRenderer : public ImlibRenderer
{
   public:	

      DummyRenderer(int x, int y, int width, int height, string cfgPath, int utf, string thmPath)
         : ImlibRenderer(x, y, width, height, cfgPath, utf, thmPath) { }

		int init(int lazy) { return ImlibRenderer::init(lazy); }
		void deinit()      { }
};

//***************************************************************************
#endif // __GTFT_DMYRENDERER_HPP__
