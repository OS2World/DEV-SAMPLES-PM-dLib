/************************************************************************
dLib v0.5
Graeme Blackley
g_blackl@kai.ee.cit.ac.nz
*************************************************************************/

#define IMAGE_NOTCOMPILED   0
#define IMAGE_COMPILED      1

struct _image {
    unsigned short xs,ys;
    unsigned char type;
    unsigned char *image;
    unsigned char *code;
};

typedef struct _image IMAGE;
typedef IMAGE *PIMAGE;

#if defined(__IBMC__)
 #define __syscall _System
#endif


PCHAR __syscall dCreateVirtualScreen(ULONG xsize, ULONG ysize);
    /* Creates the virtual screen. This must be called before any other 
       dLib functions. It is a good idea to make the virtual screen a bit
       larger than the view window. This allows scrolling, or an area 
       allowing images to be drawn partially/totally out of view.
       This function returns a pointer to the virtual screen, for use
       by the application. */

void __syscall dSetOrigin(ULONG x, ULONG y);
    /* This allows you look at different part of the virtual screen. 
       (x,y) specifies what location in the virtual screen, will appear
       at the top left of the screen. This is used for scrolling. It does 
       not take effect until the next dShowView() */

void __syscall dSetViewArea(ULONG atx, ULONG aty, ULONG xsize, ULONG ysize);
    /* Set where on the screen (monitor), in screen coordinates, the view
       is to be display. This can be a bit difficult to work out. 
       (0,0) is the top left. Affects next dShowView() */

ULONG __syscall dShowView(void);
    /* Displays the current view, and any changes made. */

void __syscall dDistroyVirtualScreen(void);
    /* Distroys Virtual screen, and free extra memory */

void __syscall dEnableDoubleBuf(void);
    /* Makes a backup copy of the virtual screen. This backup can be used
       to replace backgrounds during animation. It alot more effective
       than copying the background before each draw of an image. */

void __syscall dReplaceFromDoubleBuf(int x1, int y1, int x2, int y2);
    /* This function copies image data from the backup copy of the virtual 
       screen. It is used to replace background images during animation.
       dEnableDoubleBuf() must be called before this function. */

void __syscall dDisableDoubleBuf(void);
    /* This disables double buffering, and frees the memory used be the
       backup copy of the screen. */

void __syscall dSetVirtualPalette(PCHAR *rgb);
    /* This function initializes a virtual palette. rgb is a pointer to 
       768 bytes of data. This being 3 bytes (1 red, 1 green, 1 blue) for
       each of the 256 colors */

unsigned char __syscall dNearestColor(unsigned char index);
    /* This function returns the index of the physical color, most closely
       matching the virtual color. */

void __syscall dRealizeImages(void);
    /* Not supported yet. Its going to be for calling when another 
       application changes the palette. */

void __syscall dWaitRetrace(void);
    /* Can be used in animations to get super-smooth frames. This call is 
       very dependant on the machine speed, so use it with caution. */

PIMAGE __syscall dLoadImage(char *filename);
    /* Loads an image file from disk. This file has two 16bit words at the 
       beginning. These are the horizontal size(xs) and vertical size(ys).
       These words are followed by (xs*ys) bytes of image data. It would be
       simple to write a routine to convert .bmp files into this format.
       Theres also a couple of shareware paint programs (dos) that write
       in this format, 256paint.zip? I personally use autodesk animator
       and have writen a program that convert .cel's to this format.
       If anyone writes any OS/2 based conversion (ie gif,bmp) tools, send
       me a copy and I'll include it in a posible future release. */

void __syscall dCompileSprite(PIMAGE image);
    /* This compiles an image, loaded with dLoadImage, in to a sprite. I
       call images with transparent parts 'sprites'(not quite true). This
       could also come from any other source aslong as it correctly fills
       a IMAGE structure. */

void __syscall dBltSprite(unsigned int x, unsigned int y, PIMAGE image);
    /* Draws a 'sprite' at (x,y). All parts of the image using color zero
       will be transparent. This function may be a porters nightmare.
       Debugging inside this call may also cause nightmares. It is a very
       fast method */

void __syscall dBltImage(unsigned int x, unsigned int y, PIMAGE image);
    /* Draw and image at (x,y). No transparent parts */

void __syscall dBltStretchImage(PIMAGE src, ULONG xd1, ULONG yd1, ULONG xd2, ULONG yd2);
    /* This stretches an image to fill the rectangle specified on the
       virtual screen. (xd1,yd) being the top left, (xd2,yd2) being bottom
       right. Even though this is a well optomised stretch, its not the sort
       of call you want to make often. In my tests is much faster than
       similar OS/2 Gpi calls. Useful for stretching a picture over the 
       entire background. */

void __syscall dCircle(int x1, int y1, int radius, unsigned char color);
    /* Draws a Bresnham circle at (x,y) */

void __syscall dLine(int x1, int y1, int x2, int y2, unsigned char color);
    /* draws a line from (x1,y1) to (x2,y2) */

void __syscall dBox(int x1, int y1, int x2, int y2, unsigned char color);
    /* draws a solid box from (x1,y1) to (x2,y2) */

void __syscall dPixel(ULONG x, ULONG y, unsigned char color);
    /* draws a dot at (x,y) */

