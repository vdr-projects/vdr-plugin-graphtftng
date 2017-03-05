/*
 * scan.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *  (c) 2007-2013 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */

//***************************************************************************
// Includes
//***************************************************************************

#include <scan.h>

//***************************************************************************
// Scanner
//***************************************************************************

const char* Scan::delimiters = " ;,(){}[]\"";
const char* Scan::operators  = " +-*:<>!=";
const char* Scan::whitespace = " ()";
const char* Scan::logicalOps = "|&";
const char* Scan::numprefix = "+-";

//***************************************************************************
// Scanner
//***************************************************************************

int Scan::eat(int aANP)
{ 
   int i = 0;
   int anp = aANP != na ? aANP : allowNumPrefix;

   _last[0] = 0;
   isStr = no;

   skipWs();

   while (p && *p && i < 1000)
   {
      if (*p == '"')
      {
         if (isStr)
         {
            p++;
            break;
         }

         isStr = yes;
         p++;

         continue;
      }

      if (!isStr && isDelimiter(*p))
         break;

      if (isStr && *p == '"')
         break;

      if (i)
      {
         if (isOperator(*_last) && !isOperator(*p) && !isStr)
         {
            if (!isNum(*p))
               break;
            if (!anp || !isNumPrefix(*_last))
               break;
         }

         if (!isOperator(*_last) && isOperator(*p) && !isStr)
            break;

         if (isNum(*_last) && !isNum(*p) && !isStr)
            break;
      }

      if (*p != '"')
         _last[i++] = *p;

      p++;
   }

   _last[i] = 0;

   return i == 0 ? fail : success;
}
