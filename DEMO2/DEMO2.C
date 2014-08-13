#define INCL_WIN
#define INCL_DOS
#define INCL_GPI
#include <os2.h>
#include <stdio.h>
#include <process.h>
#include "skeleton.h"
#include "dlib.h"


#define NUM 5
struct POS {
   int x,y;
};

struct MY_DEMO {
   struct POS pos;
   int spx,spy;
   char xdir,ydir;
};


MRESULT EXPENTRY ClientWndProc (HWND,ULONG,MPARAM,MPARAM);

#define MSG_SPRITES     1001
#define MSG_SCROLL      1002
#define MSG_START       1003
#define MSG_LINES       1004
#define MSG_STRETCH     1005

#define GetYSize() ScreenHeight

HAB   hab;
HWND  hWndFrame, hWndClient;
int   ScreenHeight;
CHAR  szTitle[64];
int   TheKey;
TID   tid;

int main()
{
    HMQ   hmq;
    QMSG  qmsg;
    ULONG flFrameFlags    = FCF_TITLEBAR | FCF_SYSMENU       | FCF_DLGBORDER |
                            FCF_MINMAX   | FCF_SHELLPOSITION | FCF_TASKLIST  |
                            FCF_ICON     | FCF_MENU;
    CHAR  szClientClass[] = "CLIENT";

    hab = WinInitialize (0);
    hmq = WinCreateMsgQueue (hab, 0);

    WinRegisterClass (hab, szClientClass, (PFNWP)ClientWndProc, CS_SIZEREDRAW|CS_MOVENOTIFY, 0);
    WinLoadString (hab, 0, ID_APPNAME, sizeof(szTitle), szTitle);

    hWndFrame = WinCreateStdWindow (HWND_DESKTOP, 0,
        &flFrameFlags, szClientClass, "g_blackl@kai.ee.cit.ac.nz", 0, 0, ID_APPNAME, &hWndClient);
    WinSetWindowPos(hWndFrame,HWND_TOP,5,GetYSize()-295,370,290,SWP_SIZE |SWP_SHOW |SWP_ZORDER |SWP_MOVE | SWP_ACTIVATE);
    dCreateVirtualScreen(700,500);

    while (WinGetMsg (hab, &qmsg, 0, 0, 0))
        WinDispatchMsg (hab, &qmsg);

    WinDestroyWindow (hWndFrame);
    WinDestroyMsgQueue (hmq);
    WinTerminate (hab);
    return (0);
}



void iSprites(struct MY_DEMO *spr,char number)
/* init sprite positions and speed */
{
    char i;

    for (i=0;i<number;i++) {
       spr[i].pos.x=rand()%400;
       spr[i].pos.y=rand()%300;
       spr[i].spx=(rand()%2)+1;
       spr[i].spy=(rand()%2)+1;
       spr[i].xdir=1;
       spr[i].ydir=1;
    }
}


void uSprite(struct MY_DEMO *spr)
/* Moves the sprites, rebounding off edges */
{
    if (spr->xdir == 1) {
       spr->pos.x = spr->pos.x + spr->spx;
    } else {
       spr->pos.x = spr->pos.x - spr->spx;
    }
    if (spr->ydir == 1) {
       spr->pos.y = spr->pos.y + spr->spx;
    } else {
       spr->pos.y = spr->pos.y - spr->spx;
    }

    if (spr->pos.x > 400)
       spr->xdir = 0;
    if (spr->pos.x < 10)
       spr->xdir = 1;
    if (spr->pos.y > 340)
       spr->ydir = 0;
    if (spr->pos.y < 10)
       spr->ydir = 1;
}




void AnimThread(void *arg)
/* show the functions in action. Function descriptions in dlib.h
   NOTE: dCreateVirtualScreen and dSetViewArea have already been called. */
{
    char *scr;
    FILE *palette;
    char rgb[256*3];
    PIMAGE pic1;
    int i;
    struct MY_DEMO sprite[NUM];

    palette = fopen("bad1.pal","rb");
    fread(rgb,3,256,palette);
    fclose(palette);
    dSetVirtualPalette(rgb);                /* load and setup a virtual palette */

    pic1 = dLoadImage("bad1.vga");
    dCompileSprite(pic1);                   /* enable dBltSprite to be called */
    WinPostMsg(hWndFrame,MSG_LINES,0,0);
    for (i=0;i<256; i+=4) {
       dCircle(rand()%500,rand()%350,i,i);
       dLine(rand()%600,rand()%400,rand()%600,rand()%400,i);
    } 
    dShowView();
    DosSleep(5000);

    WinPostMsg(hWndFrame,MSG_SCROLL,0,0);
    for (i=0; i<450; i++) {                 /* scroll right */
        dSetOrigin(i,0);
        dWaitRetrace();
        dShowView();
    }
    for (i=450; i>0; i--) {                 /* scroll left */
        dSetOrigin(i,0);
        dWaitRetrace();
        dShowView();
    }

    WinPostMsg(hWndFrame,MSG_SPRITES,0,0);
    dSetOrigin(30,30);                      /* create border around view area */
    for (i=4; i<150; i+=10) {
        dBltStretchImage(pic1,30,30,30+i,30+i);
        dShowView();
    }
    dEnableDoubleBuf();                     /* prepare for animation */

    iSprites(sprite,NUM);
    while (1) {
        for (i=0;i<NUM;i++) 
          dBltSprite(sprite[i].pos.x,sprite[i].pos.y,pic1);
        dShowView();
        for (i=0;i<NUM;i++) 
          dReplaceFromDoubleBuf(sprite[i].pos.x, sprite[i].pos.y, sprite[i].pos.x+pic1->xs, sprite[i].pos.y+pic1->ys);
        for (i=0;i<NUM;i++)
          uSprite(&sprite[i]);
    }
}




void SetupViewArea(void)
{
    SWP     swp;
    RECTL   rect;
    WinQueryWindowPos( hWndFrame, (PSWP)&swp );
    rect.xLeft = swp.x;
    rect.yBottom = swp.y;
    rect.xRight = swp.x + swp.cx;
    rect.yTop = swp.y + swp.cy;
    WinCalcFrameRect(hWndFrame, &rect, TRUE);
    dSetViewArea(rect.xLeft,ScreenHeight-(rect.yTop),rect.xRight-rect.xLeft,rect.yTop-rect.yBottom);
}




MRESULT EXPENTRY ClientWndProc (HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    HPS     hps;
    BOOL    bHandled = TRUE;
    MRESULT mReturn  = 0;
    RECTL   rect;
    char    buf[50];
    POINTL  pointl = {50,50};
    APIRET  rc;
    SWP     swp;
    TID     tid;
    USHORT  flag;

    switch (msg)
    {
         case MSG_SCROLL:
            WinSetWindowText(hWndFrame,"Scrolling (moving Origin)");
            break;
 
         case MSG_LINES:
            WinSetWindowText(hWndFrame,"Lines, Circles...simple stuff");
            break;

         case MSG_SPRITES:
            WinSetWindowText(hWndFrame,"Bitmaps/Sprites/Palette Mapping");
            break;

         case WM_CREATE:
            ScreenHeight = WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN);
            WinShowWindow(hWnd, TRUE);
            _beginthread(AnimThread,NULL,32768,NULL);
            break;

         case WM_MOVE:
            SetupViewArea();
            break;

         case WM_DESTROY:
            bHandled = FALSE;
            break;

         case WM_FOCUSCHANGE:
            flag = SHORT1FROMMP(mp2);
            if (flag == TRUE) {
               DosExitCritSec();
            } else {
               DosEnterCritSec();
            } 
            bHandled = FALSE;
            WinInvalidateRect(hWnd,NULL,FALSE);
            break;

        case WM_PAINT:
            SetupViewArea();
            hps = WinBeginPaint (hWnd,0,0);
            WinQueryWindowRect(hWnd,&rect);
            WinFillRect(hps, &rect, CLR_BLACK);
            WinEndPaint (hps);
            break;

        case WM_COMMAND:
            switch (LOUSHORT(mp1))
            {
                case IDM_ABOUT:
                    DosEnterCritSec();
                    WinMessageBox(HWND_DESKTOP,
                        hWnd,
                        "This is a simple demonstration of some dLib functions. This demo"
                        " isn't as good as it could be. I wanted to keep the code short and"
                        " easy to follow."
                        "\n\nGraeme Blackley\ng_blackl@kai.ee.cit.ac.nz",
                        "dLib information",
                        0,
                        MB_INFORMATION | MB_OK);
                    DosExitCritSec();
                    break;
            }

        default:
            bHandled = FALSE;
            break;
    }

    if (!bHandled)
        mReturn = WinDefWindowProc (hWnd,msg,mp1,mp2);

    return (mReturn);
}

