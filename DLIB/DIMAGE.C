#define INCL_DEV
#define INCL_DOS
#define INCL_WIN
#define INCL_32
#include <os2.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "dlib.h"
#include "portio.h"
#define sign(x) ((x)>0 ? 1:-1)


extern PCHAR VScreen;
extern char _TranslateRGB[256];
extern ULONG VScreenX, VScreenY, VScreenSize;  



PIMAGE __syscall dGetImage(unsigned int x, unsigned int y, unsigned int xsize, unsigned int ysize)
/* Creates and image from an area of the virtual screen */
{
    int size, xpos, ypos, i = 0;
    PIMAGE tmp;
    PCHAR loc = VScreen + (y * VScreenX) + x;

    size = xsize * ysize;
    tmp = malloc(sizeof(IMAGE)); 
    tmp->image = malloc(size);
    tmp->xs = xsize;
    tmp->ys = ysize;
    for (ypos=0; ypos < ysize; ypos++) {
       for (xpos=0; xpos < xsize; xpos++) {
           tmp->image[i++] = *loc;
           loc++;
       } 
       loc += VScreenX;
    }
    return tmp;
}



void __syscall dBltImage(unsigned int x, unsigned int y, PIMAGE image)
/* Draw the picture defined by 'image' at (x,y) */
{
    PCHAR loc = VScreen + (y * VScreenX) + x;
    ULONG size = image->xs, i, src = 0;

    for (i=0; i<(image->ys); i++) {
       memcpy(loc, &image->image[src], size);
       loc += VScreenX;
       src += size;
    }
}




/*********************************** sprite execution */
unsigned int _where, _code;
extern void ExecuteSprite(void);
#pragma aux ExecuteSprite = \
     "mov eax, _code"       \
     "mov edx, _where"      \
     "call eax"             \
     modify [eax edx];


void __syscall dBltSprite(unsigned int x, unsigned int y, PIMAGE image)
/* Draw the picture defined by 'image' at (x,y). This picture 
   shows all color but zero. Zero's appear as transparent bits*/
{
   unsigned int loc=(unsigned long)VScreen + ((y*VScreenX) + x);
   _code = (unsigned)image->code;
   _where = loc;
   ExecuteSprite();
}
/*********************************** sprite execution */



void __syscall dCompileSprite(PIMAGE image)
/* Generates executable code to draw an image. This is a very good way
   to do things. This means no 'cmp' instructions are executed while
   drawing. Very fast. ie Each sprite knows the best way to draw itself */
{
   int ip=0,x,y,d=0,adder = 0;
   unsigned char tmp;

   image->code = malloc((image->xs * image->ys) *7);
   for (y=0; y<(image->ys); y++) {
      for (x=0; x<(image->xs); x++) {
         tmp = image->image[d++];
         if (tmp) {
            /* mov byte ptr [adder+edx],tmp */
            image->code[ip++] = 0xc6;
            image->code[ip++] = 0x82;
            image->code[ip++] = (adder & 0xff);
            image->code[ip++] = ((adder >> 8) & 0xff);
            image->code[ip++] = ((adder >> 16) & 0xff);
            image->code[ip++] = ((adder >> 24) & 0xff);
            image->code[ip++] = tmp;
         }
         adder++;
      }
      adder+=(VScreenX - image->xs);
   }
   image->code[ip++] = 0xc3; /* ret*/
}




void Stretch(long x1, long x2, long y1, long y2, long yr, long yw, PIMAGE src)
/* used internaly by dBltStretchImage */
{
    long dx,dy,d,dx2;
    register e,sloc,dloc;
    ULONG sx,sy,color;

    sx = sign(x2-x1);
    sy = sign(y2-y1);
    dx = abs(x2-x1);
    dy = abs(y2-y1);
    e = (dy<<1)-dx;
    dloc = yw * VScreenX;
    sloc = yr * src->xs;
    dx2 = dx<<1;
    dy <<= 1;

    for(d=0; d<=dx; d++) {
        VScreen[x1 + dloc] = src->image[y1 + sloc];
        while (e >= 0) {
            y1 += sy;
            e -= dx2;
        }
        x1 += sx;
        e += dy;
    }
}


void __syscall dBltStretchImage(PIMAGE src, ULONG xd1, ULONG yd1, ULONG xd2, ULONG yd2)
/* This stretches an image to fill the rectangle specified on the
   virtual screen. (xd1,yd) being the top left, (xd2,yd2) being bottom
   right. Even though this is a well optomised stretch, its not the sort
   of call you want to make often. In my tests is much faster than
   similar OS/2 Gpi calls. Useful for stretching a picture over the
   entire background. */
{
    long dx, dy, e, d, dx2, xs1 = 0, ys1 = 0, xs2, ys2;
    int sx,sy;

    xs1 = 0;
    ys1 = 0;
    xs2 = src->xs;
    ys2 = (src->ys-1);
    sx = sign(yd2 - yd1);
    sy = sign(ys2 - ys1);
    dx = abs(yd2 - yd1);
    dy = abs(ys2 - ys1);
    e = (dy<<1) - dx;
    dx2 = dx<<1;
    dy <<= 1;

    for(d=0; d<=dx; d++){
        Stretch(xd1, xd2, xs1, xs2, ys1, yd1, src);     
        while (e >= 0) {
            ys1 += sy;
            e -= dx2;
        }
        yd1 += sx;
        e += dy;
    }
}


PIMAGE __syscall dLoadImage( char *filename )
/* Loads an image from disk. The file has 2 shorts at the beginning.
   These are the Xsize and Ysize. The rest of the file should be
   Xsize * Ysize of color bytes. */
{
   FILE *img;
   PIMAGE tmp;
   int i;

   img = fopen(filename,"rb");
   if (img == NULL)
       return NULL;

   tmp = malloc(sizeof(IMAGE));
   fread(&tmp->xs, sizeof(USHORT), 1, img);
   fread(&tmp->ys, sizeof(USHORT), 1, img);
   tmp->image = malloc(tmp->xs * tmp->ys);
   fread(tmp->image, (tmp->xs * tmp->ys), sizeof(char), img);
   for (i=0; i<(tmp->xs * tmp->ys); i++)
        tmp->image[i] = _TranslateRGB[tmp->image[i]];
   fclose( img );
   return tmp;
}


