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

struct {
  ULONG ulPhysicalAddress;              /* Physical address                         */
  ULONG ulApertureSize;                 /* 1 Meg, 4 Meg or 64k                      */
  ULONG ulScanLineSize;                 /* This is >= the screen width in bytes.    */
  RECTL rctlScreen;                     /* Device independant co-ordinates          */
} ApertureInfo;

HAB   hab;                              /* Need a handle to an allocation block.      */
HDC   hdc;                              /* Need a device context for the DecEsc's     */
PBYTE pbLinearAddress;                  /* Holds the linear address to the video card.*/
ULONG ulTotalScreenColors= 0L;          /* Holds the total number of colors.          */
HFILE hDeviceDriver;                    /* Handle to the device driver to do mapping. */
PCHAR VScreen;                          /* Pointer to Virtual Screen                  */
PCHAR Dbuf = NULL;                      /* 2nd virtual screen for double buffering    */
ULONG VScreenX, VScreenY, VScreenSize;  /* Virtual screen details                     */
ULONG VHeight, VWidth;                  /* View width and height                      */
ULONG OriginX = 0, OriginY = 0;         /* View origin x and y (within virtual screen)*/
ULONG Vatx = 0, Vaty = 0;               /* where window will be on screen             */
CHAR  VPalette[768];                    /* Copy of Virtual Palette                    */
CHAR  PPalette[768];                    /* Copy of Physical Palette                   */
CHAR  _TranslateRGB[256];               /* RGB <--> RGB color translation table       */
ULONG _startbank;
PBYTE _startaddr;
PBYTE _Ostartaddr;
ULONG _ulAperture;
void ( *DisplayFunction )( void );      /* which blt method to use....                */


struct _COLOR_CHANGE_ITEM {             /* list of images needing color translation...*/
   ULONG size;                          /* ..... in case of palette change            */
   ULONG type;
   PCHAR image;
   struct _COLOR_CHANGE_ITEM *next;
} ImageListHead = {0,NULL,NULL};



extern void memcpyd(char *dest,char *src,ULONG size);
#pragma aux memcpyd = \
    "shr ecx,2"       \
    "rep movsd"       \
    parm [edi][esi][ecx];



void _segmented_aperture_256(void)
/* used internally by dlib, for 256 color modes, with 64k aperture */
{
   #define DEVESC_ACQUIREFB   33010L
   #define DEVESC_DEACQUIREFB 33020L
   #define DEVESC_SWITCHBANK  33030L
   ULONG ulNumBytes;
   PBYTE pbSrc = _Ostartaddr;
   ULONG rc = 0L;
   ULONG X, Y, ulAperture = _ulAperture;
   PBYTE pbDst = _startaddr;
   struct {
          ULONG  fAFBFlags;
          ULONG  ulBankNumber;
          RECTL  rctlXRegion;
          } acquireFb;

   acquireFb.rctlXRegion.xLeft   = 0L;
   acquireFb.rctlXRegion.xRight  = 0L;
   acquireFb.rctlXRegion.yTop    = 0L;
   acquireFb.rctlXRegion.yBottom = 0L;
   acquireFb.ulBankNumber        = _startbank;
   acquireFb.fAFBFlags           = 1L;  /* Acquire and switch simultaneously.*/

   if ( DevEscape ( hdc, DEVESC_ACQUIREFB, sizeof (acquireFb),
            (PBYTE) &acquireFb, (PLONG) &ulNumBytes, (PBYTE) NULL ) !=DEV_OK )
      return;

   Y = VHeight;
   while ( Y-- ) {
       /* Check if this line will pass through an aperture switch.      */
       if ( ulAperture < VWidth ) {
           /* Move the rest of bytes for this aperture to the screen.    */
           memcpyd ( pbDst, pbSrc, ulAperture );
           pbDst += VWidth;
           pbSrc += VScreenX;

           /* Now I need to do a bank switch.                            */
           acquireFb.ulBankNumber++;
           DevEscape ( hdc, DEVESC_SWITCHBANK, 4L, (PBYTE) &acquireFb.ulBankNumber,(PLONG) &ulNumBytes, (PBYTE) NULL );

           /* Set up the rest of the line to move to the screen.         */
           X = VWidth - ulAperture;

           /* Reset the linear address to the begining of the bank.      */
           pbDst = pbLinearAddress;
           ulAperture = ApertureInfo.ulApertureSize + ulAperture;
       } else
           /* There's room on this aperture for the whole line, so do it.*/
           X = VWidth;

       /* Move the pixels to the screen.                                */
       memcpyd ( pbDst, pbSrc, X );
       pbSrc += VScreenX;
       pbDst += X;

       /* Adjust the output line destination.                           */
       pbDst += ApertureInfo.ulScanLineSize - VWidth;
       ulAperture -= ApertureInfo.ulScanLineSize;
       /* If aperture size is non-positive, then we do a switch banks.  */
       if ((LONG)ulAperture <= 0 ) {
           ulAperture += ApertureInfo.ulApertureSize;
           pbDst -= ApertureInfo.ulApertureSize;
           acquireFb.ulBankNumber++;
           DevEscape( hdc, DEVESC_SWITCHBANK, 4L,(PBYTE) &acquireFb.ulBankNumber,(PLONG) &ulNumBytes,(PBYTE) NULL);
       }
   }
   /* Release the frame buffer... this is important.                         */
   DevEscape ( hdc, DEVESC_DEACQUIREFB, (ULONG)0L, (PBYTE) NULL,(PLONG)0L, (PBYTE) NULL );
   return;
}



void _large_aperture_256(void)
/* used internally by dlib, for 256 color modes, with linear mode */
{
   #define DEVESC_ACQUIREFB   33010L
   #define DEVESC_DEACQUIREFB 33020L
   #define DEVESC_SWITCHBANK  33030L
   ULONG ulNumBytes;
   PBYTE pbSrc = _Ostartaddr;
   ULONG rc = 0L;
   ULONG X, Y, ulAperture = _ulAperture;
   PBYTE pbDst = _startaddr;
   struct {
          ULONG  fAFBFlags;
          ULONG  ulBankNumber;
          RECTL  rctlXRegion;
          } acquireFb;

   acquireFb.rctlXRegion.xLeft   = 0L;
   acquireFb.rctlXRegion.xRight  = 0L;
   acquireFb.rctlXRegion.yTop    = 0L;
   acquireFb.rctlXRegion.yBottom = 0L;
   acquireFb.ulBankNumber        = 0;
   acquireFb.fAFBFlags           = 1L;  /* Acquire and switch simultaneously.*/

   if ( DevEscape ( hdc, DEVESC_ACQUIREFB, sizeof (acquireFb),
            (PBYTE) &acquireFb, (PLONG) &ulNumBytes, (PBYTE) NULL ) !=DEV_OK )
      return;

   Y = VHeight;
   while ( Y-- ) {
       /* Move the pixels to the screen.                                */
       memcpyd ( pbDst, pbSrc, VWidth );
       pbSrc += VScreenX;
       pbDst += ApertureInfo.ulScanLineSize;
   }
   /* Release the frame buffer... this is important.                         */
   DevEscape ( hdc, DEVESC_DEACQUIREFB, (ULONG)0L, (PBYTE) NULL,(PLONG)0L, (PBYTE) NULL );
   return;
}



ULONG __syscall dShowView(void)
{
    DisplayFunction();
    return 0UL;
}




ULONG MapPhysicalToLinear ( ULONG ulPhysicalAddress )
/* used internally by dlib */
{
   ULONG  ulActionTaken;
   ULONG  ulDLength;
   ULONG  ulPLength;
   struct {
          ULONG   hstream;
          ULONG   hid;
          ULONG   ulFlag;
          ULONG   ulPhysAddr;
          ULONG   ulVram_length;
          } parameter;
   #pragma pack (1)
   struct {
          USHORT usXga_rng3_selector;
          ULONG  ulLinear_address;
          } ddstruct;
   #pragma pack ()

   /* Attempt to open up the device driver.                                  */
   if ( DosOpen( (PSZ)"\\DEV\\SMVDD01$", (PHFILE) &hDeviceDriver,
            (PULONG) &ulActionTaken, (ULONG)  0L, (ULONG) FILE_SYSTEM,
            OPEN_ACTION_OPEN_IF_EXISTS, OPEN_SHARE_DENYNONE  |
            OPEN_FLAGS_NOINHERIT | OPEN_ACCESS_READONLY, (ULONG) 0L)   )
      return ( 3L );

   /* Set up the parameters for the IOCtl to map the vram to linear addr.    */
   parameter.hstream       = 0L;
   parameter.hid           = 0L;
   parameter.ulFlag        = 1L;     /* Meaning MapRam. */
   parameter.ulPhysAddr    = ulPhysicalAddress;
   parameter.ulVram_length = ApertureInfo.ulApertureSize;
   ulPLength               = sizeof (parameter);
   ulDLength               = 0L;

   /* Call the IOCtl to do the map.                                          */
   if ( DosDevIOCtl ( hDeviceDriver, (ULONG)0x81,
                      (ULONG)0x42L, (PVOID)&parameter,
                      (ULONG)ulPLength, (PULONG)&ulPLength,
                      (PVOID)&ddstruct, (ULONG)6, (PULONG)&ulDLength ) )
      return ( 4L );

   /* Set the variable to the linear address, and return.                    */
   pbLinearAddress= (PBYTE) ddstruct.ulLinear_address;

   return ( 0L );
}





ULONG DirectScreenInit ( VOID )
/* used internally by dlib, see DIVE.ZIP on ftp-os2.cdrom.com for more details */
{
   #define DEVESC_GETAPERTURE 33000L
   ULONG          rc;
   ULONG          ulFunction;
   LONG           lOutCount;
   HPS            hps;
   DEVOPENSTRUC   dop= {0L,(PSZ)"DISPLAY",NULL,0L,0L,0L,0L,0L,0L};

   if ( ulTotalScreenColors )
      return ( 0xffffffff );

   DisplayFunction = NULL;
   hab= WinInitialize ( 0L );
   hps= WinGetPS ( HWND_DESKTOP );
   hdc= DevOpenDC ( hab, OD_MEMORY, (PSZ)"*", 5L,(PDEVOPENDATA)&dop, (HDC)NULL);
   DevQueryCaps ( hdc, CAPS_COLORS, 1L, (PLONG)&ulTotalScreenColors);

   ulFunction = DEVESC_GETAPERTURE;
   if ( DevEscape( hdc, DEVESC_QUERYESCSUPPORT, 4L,(PBYTE)&ulFunction, NULL, (PBYTE)NULL ) == DEV_OK ){
      lOutCount= sizeof (ApertureInfo);
      if ( DevEscape ( hdc, DEVESC_GETAPERTURE, 0L, (PBYTE) NULL,
            &lOutCount, (PBYTE)&ApertureInfo) != DEV_OK ){
         ulTotalScreenColors= 0L;
         rc= 2L;
      }else{
         if ( ulTotalScreenColors==16L ){
            ApertureInfo.ulScanLineSize= ApertureInfo.ulScanLineSize >> 1;
/*          DisplayFunction = ;   <-- 4 bit color routine*/
         } else if ( ulTotalScreenColors==256L ) {
            if (ApertureInfo.ulApertureSize > 65536) {
               DisplayFunction = _large_aperture_256;
            } else {
               DisplayFunction = _segmented_aperture_256;
            } 
         } else if ( ulTotalScreenColors==65536L ){
            ApertureInfo.ulScanLineSize= ApertureInfo.ulScanLineSize << 1;
/*          DisplayFunction = ;   <--16 bit color routine*/
         }else if ( ulTotalScreenColors==16777216L ) {
            ApertureInfo.ulScanLineSize= ApertureInfo.ulScanLineSize +
            ( ApertureInfo.ulScanLineSize << 1 );
/*          DisplayFunction = ;   <--24 bit color routine*/
         }
         rc= 0L;
      }
   } else {
      ulTotalScreenColors= 0L;
      rc= 1L;
   }

   WinReleasePS ( hps );
   if ( !rc )
      rc = MapPhysicalToLinear ( ApertureInfo.ulPhysicalAddress );

   return ( rc );
}




ULONG DirectScreenTerm ( VOID )
/* used internally by dlib */
{
   if ( !ulTotalScreenColors )
      return ( 1L );
   ulTotalScreenColors= 0L;
   DevCloseDC ( hdc );
   WinTerminate ( hab );
   return ( DosClose ( hDeviceDriver ) );
}





void __syscall dWaitRetrace(void)
/* returns in the next verticle retrace, could be used for smooth animation. */
{
   while ((inp(0x3da)&0x08)!=0);        /* poll VR bit */
   while ((inp(0x3da)&0x08)==0);
}





PCHAR __syscall dCreateVirtualScreen( ULONG xsize, ULONG ysize )
/* Creates a virtual screen, and returns a pointer to it. This MUST be
   called before any other dlib functions */
{
    int i;

    if (DirectScreenInit() != 0)        /* retrieve MMPM/2 SMV details and int */
        return NULL;
    VScreenX = xsize;
    VScreenY = ysize;
    VScreenSize = xsize * ysize;
    VScreen = (PCHAR)malloc( VScreenSize );
    for (i=0; i<256; i++)               /* set colors to the OS/2 default palette */
        _TranslateRGB[i] = i;
    return VScreen;
}


void __syscall dSetOrigin(ULONG x, ULONG y)
/*  (x,y) sets the offset into the virtual screen. Can be used for scrolling.*/
{
    if (x > (VScreenX - VWidth))
       x = (VScreenX - VWidth);
    if (y > (VScreenY - VWidth))
       y = (VScreenY - VWidth);

    OriginX = x;
    OriginY = y;
    _Ostartaddr = VScreen + OriginX + (OriginY * VScreenX);
}


void __syscall dSetViewArea( ULONG atx, ULONG aty, ULONG xsize, ULONG ysize )
/* This set the information about the view window. 
   (atx,aty)     give the position on the screen. (0,0) is the top left of the screen.
   (xsize,ysize) is the size of the window to display. */
{
    if (xsize > VScreenX)
       xsize = VScreenX;
    if (ysize > VScreenY)
       ysize = VScreenY;

    VHeight = ysize;                    /* size of viewport into virtual screen */
    VWidth = xsize;
    Vatx = atx;                         /* location on screen */
    Vaty = aty;

    if (ApertureInfo.ulApertureSize == 0)
       ApertureInfo.ulApertureSize = 64000;
    
    _startbank = (Vaty * ApertureInfo.ulScanLineSize) / ApertureInfo.ulApertureSize;
    _startaddr = pbLinearAddress + Vatx + ((Vaty * ApertureInfo.ulScanLineSize) % ApertureInfo.ulApertureSize);
    _ulAperture = ApertureInfo.ulApertureSize - ((Vaty * ApertureInfo.ulScanLineSize) % ApertureInfo.ulApertureSize);
    _Ostartaddr = VScreen + OriginX + (OriginY * VScreenX);
}





#define SQR(x) ((x) * (x))
int rgb_diff(int r1,int g1,int b1,int r2,int g2,int b2)
/* Used internally by dlib to setup the color translation table */
{
   int tmp, rd, gd, bd;
   rd = abs(r1 - r2);
   gd = abs(g1 - g2);
   bd = abs(b1 - b2);
   tmp = SQR((rd)) + SQR((gd)) + SQR((bd));
   return abs(tmp);
}



void SetupTranslationTable(void)
/* Used internally by dlib to setup the color translation table */
{
   unsigned int c, i, min, conv2, dif;
 
   for (c=0; c<256; c++) {
        outp(0x3c7, c);
        PPalette[c*3] = inp(0x3c9);
        PPalette[(c * 3) + 1] = inp(0x3c9);
        PPalette[(c * 3) + 2] = inp(0x3c9);
   } 
   for (c=1; c<256; c++) {
      min = 0xffffff;
      for (i=1; i<256; i++) {
         dif = rgb_diff(VPalette[(c*3)], VPalette[(c*3)+1], VPalette[(c*3)+2], 
                        PPalette[(i*3)], PPalette[(i*3)+1],PPalette[(i*3)+2]);
         if (dif < min) {
            min = dif;
            conv2 = i;
         }
      }
      _TranslateRGB[c] = conv2;
   } 
}




void __syscall dSetVirtualPalette(PCHAR *rgb)
/* Tells dlib to use a virtual palette. This isn't perfect, but usually 
   does a good job matching palettes. */
{
    memcpy( VPalette, rgb, 768 );
    SetupTranslationTable();
}




unsigned char __syscall dNearestColor(unsigned char index)
/* returns the number of the physical color most closely matching the 
   color requested from the 'virtual palette' */
{
    return _TranslateRGB[index];
}




void __syscall dRealizeImages(void)
/* This function should be called when the palette is changed. 
   When the palette is changed, WM_REALIZEPALETTE is sent to all
   PM message queues to inform of the change. */
{
    struct _COLOR_CHANGE_ITEM *list;
    int i;

    SetupTranslationTable();            /* create translation lookup table */
    list = ImageListHead.next;
    while (list != NULL) {
       for (i=0; i<(list->size); i++) {
          list->image[i] = _TranslateRGB[list->image[i]];
       }
    }
    for (i=0; i<VScreenSize; i++) {
        VScreen[i] = _TranslateRGB[VScreen[i]];
    }
    dShowView();                        /* redisplay view, with new palette */
}




void __syscall dDistroyVirtualScreen(void)
/* Free the memory held by the Virtual Screen, and close our device context */
{
    free(VScreen);
    DirectScreenTerm();
}




void __syscall dEnableDoubleBuf(void)
/* When called, this takes a copy of everything in the 'Virtual Screen'.
   This copy can be used to restore parts of the screen when needed. */
{
    Dbuf = malloc(VScreenX * VScreenY);
    memcpy(Dbuf, VScreen, (VScreenX * VScreenY));
}




void __syscall dReplaceFromDoubleBuf(int x1, int y1, int x2, int y2)
/* dEnableDoubleBuf must be called before this function. This function
   is used to restore store bits(backgrounds) of the screen, during 
   animation. Assumes (x1,y1) is top right, (x2,y2) is bottom left */
{
    ULONG loc = x1 + (y1 * VScreenX);
    ULONG size = x2 - x1;

    if (Dbuf == NULL)                   /* is double buffering active? */
       return;
    while (y1 < y2) {                   
       memcpy(&VScreen[loc], &Dbuf[loc], size);
       loc += VScreenX;
       y1++;
    }
}




void __syscall dDisableDoubleBuf(void)
/* Frees the memory used by double buffering, and disable's restores */
{
    free(Dbuf);
    Dbuf = NULL;
}


/*
void loadpal(char *filename)
{
    FILE *pal;
    char rgb[768];

    pal = fopen(filename,"rb");
    fread(rgb,768,1,pal);
    fclose(pal);
    dSetVirtualPalette(rgb);
}


int main(void)
{
    char *scr;
    PIMAGE pic1;
    int i;

    scr = dCreateVirtualScreen(1024,400);
    loadpal("test.pal");
    pic1 = dLoadImage("bad1.vga");
    dCompileSprite(pic1);
    for (i=0;i<150; i++) {
       dCircle(150,150,i,i);
    }
    for (i=0; i<400; i+=10) {
       dLine(300+i,10,700,10+i,i);
    }
    dBox(800,10,900,100,3);
    dSetViewArea(100,100,320,200);
    dEnableDoubleBuf();
    for (i=30; i<150; i+=5) {
        dBltStretchImage(pic1,10,10,i,i);
        dShowView();
    }
    while (!kbhit()) {
        for (i=0; i<600; i++) {
            dBltSprite(i,10,pic1);
            dShowView();
            dReplaceFromDoubleBuf(i, 10, i+pic1->xs, 10+pic1->ys);
        }
    }
    dDisableDoubleBuf();
    dDistroyVirtualScreen();
    getch();
    return 0;
}
*/

