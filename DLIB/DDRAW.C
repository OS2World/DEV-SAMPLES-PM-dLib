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

extern PCHAR VScreen;
extern ULONG VScreenX, VScreenY, VScreenSize;



void __syscall dPixel(ULONG x, ULONG y, unsigned char color)
/* Plane a dot at (x,y) with color...., with clipping */
{
    color = dNearestColor(color);
    if (x > VScreenX || y > VScreenY) {
       return;
    } 
    VScreen[(y*VScreenX) + x] = color;
}





#define dInlinePixel(x,y,c)     VScreen[((y)*VScreenX) + (x)] = color
/* ...compiler can optomise this better, but it has no clipping. */





void __syscall dLine(int x1, int y1, int x2, int y2, unsigned char color)
/* Draw a line from (x1,y1) to (x2,y2) with color. 
   Bresnham's line algorithm. Fairly fast. */
{
   unsigned int x,y;
   int d,dx,dy,incrE,incrNE;

   color = dNearestColor(color);
   if (x1>x2) {
     x = x2;  x2 = x1;  x1 =x;
     y = y2;  y2= y1;  y1 = y;
   } else {
     x = x1;
     y = y1;
   }

   dx = x2 - x1;
   dy = y2 - y1;
   if (abs(dx) >= abs(dy)) {
      if (dy >= 0) {
        dPixel(x, y, color);
        d = (dy<<1) - dx;
        incrE = dy<<1;
        incrNE = (dy-dx)<<1;
        while (x < x2){
           if (d <= 0){
              d += incrE;
              x++;
           } else {
              d += incrNE;
              x++;
              y++;
           }
           dPixel(x, y, color);
        }
      } else {
        dPixel(x, y, color);
        d = (dy<<1)+dx;
        incrE = dy<<1;
        incrNE = (dy+dx)<<1;
        while (x < x2) {
           if (d>=0) {
               d+=incrE;
               x++;
           } else {
               d+=incrNE;
               x++;
               y--;
           }
           dPixel(x,y,color);
        }
      }
  } else {
      if (dy>=0) {
        dPixel(x,y,color);
        d=(dx<<1) - dy;
        incrE=dx<<1;
        incrNE=(dx-dy)<<1;
        while (y<y2) {
           if (d<=0) {
              d+=incrE;
              y++;
           } else {
              d+=incrNE;
              y++;
              x++;
           }
           dPixel(x,y,color);
        }
      } else {
        dPixel(x,y,color);
        d=(dx<<1)+dy;
        incrE=dx<<1;
        incrNE=(dy+dx)<<1;
        while (y>y2) {
           if (d<=0) {
              d+=incrE;
              y--;
           } else {
              d+=incrNE;
              y--;
              x++;
           }
           dPixel(x,y,color);
        }
      }
   }
}




void _points(int x, int y, USHORT xc, USHORT yc, CHAR color)
/* This is used in drawing circles, in draws in 8 quadrents at a time.
    This makes the circle drawing calculations quite quick */
{
   dPixel(x+xc, y+yc, color);
   dPixel(y+xc, x+yc, color);
   dPixel(y+xc, -x+yc, color);
   dPixel(x+xc, -y+yc, color);
   dPixel(-x+xc, -y+yc, color);
   dPixel(-y+xc, -x+yc, color);
   dPixel(-y+xc, x+yc, color);
   dPixel(-x+xc, y+yc, color);
}



void __syscall dCircle(int x1, int y1, int radius, unsigned char color)
/* dCircle -- draws circles using integer math. This speeds the whole
    process greatly. The only multiplies are * 2's, which the compiler
    should optomize's to left shifts. */
{
  int x=0,y=radius,d=1-radius;

  color = dNearestColor(color);
  _points(x,y,x1,y1,color);             /* draw 8 quadrants */
  while (y>x) {
    if (d<0) {
       d+=2*x+3;
       x++;
    } else {
       d+=2*(x-y)+5;
       x++;
       y--;
    }
    _points(x,y,x1,y1,color);            /* draw 8 quadrants */
  }
}




void __syscall dBox(int x1, int y1, int x2, int y2, unsigned char color)
/* Draws a solid box using color, from (x1,y1) to (x2,y2). 
   This assumes (x1,y1) is top right, (x2,y2) is bottom left */
{
    PCHAR loc = VScreen + (y1 * VScreenX) + x1;
    ULONG size = x2 - x1;
 
    color = dNearestColor(color);
    while (y1 < y2) {
       memset(loc, color, size);
       loc+=VScreenX;
       y1++;
    }
}
