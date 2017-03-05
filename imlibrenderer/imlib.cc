//
// Test
//

#include <stdlib.h>
#include <stdio.h>

#include <Imlib2.h>

int text(int x, int y, const char* text, const char* fontName, int size);
Imlib_Image theImage; 

//-----------------------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------------------

int main(int argc, char** argv)
{
   if (argc != 4)
   {
      printf("Usage: imlib <text> <font-with-path> <fontsize>\n");
      return -1;
   }

   theImage = imlib_create_image(600, 100);
   imlib_context_set_image(theImage);

   text(10, 10, argv[1], argv[2], atoi(argv[3]));

   text(10, 50, "Test1 \n Test2", argv[2], atoi(argv[3]));

   imlib_image_set_format("png");
   imlib_save_image("test.png");

   imlib_free_image();

   return 0;
}

//-----------------------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------------------

int text(int x, int y, const char* text, const char* fontName, int size)
{
   char* font;
   Imlib_Font theFont;
   int height, width;

   if (!text || !fontName || !size)
      return -1;

   asprintf(&font, "%s/%d", fontName, size);

   if (!(theFont = imlib_load_font(font)))
   {
      printf("Loading font '%s' failed\n", font);
      free(font);

      return -1;
   }

   imlib_context_set_font(theFont);
   imlib_context_set_image(theImage);
   imlib_context_set_color(150, 150, 150, 255);

   imlib_get_text_size(text, &width, &height);

   printf("width (%d); height (%d)\n", width, height);
   printf("Ascent=%d/%d Descent=%d/%d\n", imlib_get_font_ascent(),
          imlib_get_maximum_font_ascent(), imlib_get_font_descent(),
          imlib_get_maximum_font_descent());

   imlib_text_draw(x, y, text);

   imlib_free_font();
   free(font);

   return 0;
}

