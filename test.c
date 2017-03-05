
#define __USE_POSIX199309

#include <sys/time.h>
#include <sys/vfs.h>
#include <time.h>

#include <unistd.h>

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <string>

#include <scan.h>

using std::string;

#define success 0
#define TB 1

//***************************************************************************
// Variable Provider
//***************************************************************************

class VariableProvider
{
   public:
      
      VariableProvider() {};
      virtual ~VariableProvider() {};

      int calcExpression(const char* expression);
      int calc(const char* op, int left, int right);
};

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
// tescht
//***************************************************************************

#include <cstdlib>
#include <iostream>

int main(int argc, char** argv)
{
   int res = 0;
   VariableProvider v;

   // res = v.calcExpression(argv[1]);
   // printf("result = %d\n", res);

   int x = 250;
   int width = 400;
   int dspWidth = 1360;
   int themeWidth = 1600;

   double xScale = (double)dspWidth / (double)themeWidth;

   int xDsp = x * xScale;
   int widthDsp = width * xScale;

   printf("%d / %d = %.2f \n", dspWidth, themeWidth, xScale);
   printf("%d / %.2f = %d \n", width, xScale, widthDsp);

   return 0;
}
