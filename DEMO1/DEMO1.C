#define INCL_WIN
#define INCL_DOS
#define INCL_GPI
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <process.h>
#include "skeleton.h"
#include "dlib.h"



MRESULT EXPENTRY ClientWndProc (HWND,ULONG,MPARAM,MPARAM);

#define MSG_SPRITES     1001
#define MSG_SCROLL      1002
#define MSG_START       1003
#define MSG_LINES       1004
#define MSG_STRETCH     1005

#define GetYSize() ScreenHeight

HAB    hab;
HWND   hWndFrame, hWndClient;
int    ScreenHeight;
int    ClientXS, ClientYS;
PIMAGE pic1;
CHAR   szTitle[64];
TID    tid;
time_t t1,t2;
ULONG  fps;

int main()
{
    HMQ   hmq;
    QMSG  qmsg;
    ULONG flFrameFlags    = FCF_TITLEBAR | FCF_SYSMENU       | FCF_SIZEBORDER |
                            FCF_MINMAX   | FCF_SHELLPOSITION | FCF_TASKLIST   |
                            FCF_ICON     | FCF_MENU;
    CHAR  szClientClass[] = "CLIENT";

    hab = WinInitialize (0);
    hmq = WinCreateMsgQueue (hab, 0);

    WinRegisterClass (hab, szClientClass, (PFNWP)ClientWndProc, CS_SIZEREDRAW|CS_MOVENOTIFY, 0);
    WinLoadString (hab, 0, ID_APPNAME, sizeof(szTitle), szTitle);

    hWndFrame = WinCreateStdWindow (HWND_DESKTOP, 0,
        &flFrameFlags, szClientClass, "g_blackl@kai.ee.cit.ac.nz", 0, 0, ID_APPNAME, &hWndClient);
    WinSetWindowPos(hWndFrame,HWND_TOP,5,GetYSize()-295,370,290,SWP_SIZE |SWP_SHOW |SWP_ZORDER |SWP_MOVE | SWP_ACTIVATE);
    dCreateVirtualScreen(1024,768);    

    while (WinGetMsg (hab, &qmsg, 0, 0, 0))
        WinDispatchMsg (hab, &qmsg);

    WinDestroyWindow (hWndFrame);
    WinDestroyMsgQueue (hmq);
    WinTerminate (hab);
    return (0);
}



void AnimThread(void *arg)
{
    char *scr;
    FILE *palette;
    char rgb[256*3];
    int OldXS,OldYS;
    int i;

    palette = fopen("bad1.pal","rb");
    fread(rgb,3,256,palette);               /* 256 colors, 1 byte red, 1 green, 1 blue */
    fclose(palette);
    dSetVirtualPalette(rgb);                         

    pic1 = dLoadImage("bad1.vga");
    dCompileSprite(pic1);    
    dBltStretchImage(pic1,0,0,ClientXS,ClientYS);
    OldXS = ClientXS;
    OldYS = ClientYS;
    dShowView();

    t1 = time(NULL);
    while (1) {
       fps++;
       dShowView();
       if (OldYS != ClientYS || OldXS != ClientXS) {
           dBltStretchImage(pic1,0,0,ClientXS,ClientYS);
           OldXS = ClientXS;
           OldYS = ClientYS;
           t1 = time(NULL);
           fps = 0;
        }
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
    ClientXS = rect.xRight-rect.xLeft;
    ClientYS = rect.yTop-rect.yBottom;
    dSetViewArea(rect.xLeft,ScreenHeight-(rect.yTop),ClientXS,ClientYS);
}



MRESULT EXPENTRY ClientWndProc (HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    HPS     hps;
    BOOL    bHandled = TRUE;
    MRESULT mReturn  = 0;
    RECTL   rect;
    char    buf[50];
    POINTL  pointl = {50,50};
    USHORT  flag;

    switch (msg)
    {
         case WM_CREATE:
            ScreenHeight = WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN);
            _beginthread(AnimThread,NULL,16556,NULL);
            WinStartTimer(hab,hWnd,109,5000);
            WinShowWindow(hWnd, TRUE);
            break;

         case WM_TIMER:
            t2 = time(NULL);
            sprintf(buf,"max fps %d, size (%d,%d)",(fps/(t2-t1)),ClientXS,ClientYS);
            WinSetWindowText(hWndFrame,buf);
            break;

         case WM_MOVE:
            SetupViewArea();
            break;

         case WM_SIZE:
            SetupViewArea();
            break; 
 
         case WM_DESTROY:
            bHandled = FALSE;
            break;

         case WM_FOCUSCHANGE:
            flag = SHORT1FROMMP(mp2);
            if (flag == TRUE) {
               DosExitCritSec();
               t1 = time(NULL);
               fps = 0;
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
                        "This is a simple demonstration of the frame rate posible"
                        " for your video card at the current window size, using dLib. "
                        "This is simply redrawing the frame over and over, as fast as possible."
                        "\n\nWhilst dLib is quite fast, it cant work miracles. If you"
                        " have a large window, or slow video card, things well look"
                        " jerky.\nGraeme Blackley\ng_blackl@kai.ee.cit.ac.nz",
                        "dLib information",    
                        0,                     
                        MB_INFORMATION | MB_OK);
                    DosExitCritSec();
                    t1 = time(NULL);
                    fps = 0;
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