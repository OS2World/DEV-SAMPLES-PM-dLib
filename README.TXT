dLib v0.5



dLib is simple, sort of fast, graphics library for OS/2. It requires MMPM/2
software motion video to be installed. It currently only runs on 256 color
OS/2 display drivers. My video card (ET4000/W32) has no high color drivers,
so I couldn't test higher colors. I decided to leave them out rather than
guess. I will add them later if I manage to borrow a card that has OS/2 
drivers for greater than 256 colors.

dLib is supplied as a .DLL, so that programs written today, with no 64k
color support, or 16.8m color, or 16 color, will work on these number of
colors as I add support for them. I may not add 16 color support. 
Newer .DLL's will be faster, with added functionality, whilst still being 
compatiable with older programs.

It uses the DIVE interface. ATI 8514 drivers dont support the video
aperture, so they dont work, sorry. There may be others that don't work,
I don't know. If you do code changes to make it work, send me a copy and 
I'll include it in any posible future releases.

This code was thrown together (over a week) from bits and pieces of
code I use for my own demos. I thought I'd share it with others. Hopefully
it may benefit the OS/2 community in some way.


The demos-(nothing too startling, I wanted to keep the code easy to follow)
DEMO1.EXE
    This is a simple demo that shows what frame rate is posible with what
    size window/video card. The frame rate is displayed in the title bar.
    I'd be interested in knowing the frame rates are on various cards.

DEMO2.EXE
    This demo just shows a few of its features in actions. Simple scrolling,
    lines and circles, and bitmaps/sprites/animation. This demo was kept
    short, so others could follow the code very easily.

DEMO3.EXE
    Shows a rotation API that will be supported in future releases.
 


My code is written with Watcom C/C++ v10.0, but should be compatiable with
most compilers, except for a few of the bitmap routines. The codes there,
feel free to port to other compilers. Portio.* files are based on the files
in portio.zip on ftp-os2.cdrom.com. They are slightly modified to work with
Watcom C. I didn't include the source, because its not my code. The complete
package is available in /os2/2_x/program/portio2.zip, ftp-os2.cdrom.com.

I have compiled the examples with both the CSet++ and the Watcom compilers.
The library appears to be compatiable. Mabey someone could let me know if
it works with Borland C.

The dLib functions are documented in dlib.h.

Although this was intended for Graphics stuff running in a PM window, DIVE
also works for fullscreen video modes(eg 320x200) with a few small changes.

In my routines I've decided to use a 'virtual' screen in memory (with blts
to screen) rather than direct screen writes. The added complexity of direct
screen writes would have slowed the code too much, ie checking for the end
of the video aperture. At the heart of my code, is bits of code from
Linden DeCarmo's DIVE.ZIP, off ftp-os2.cdrom.com (used with permission).

I've only included simple functions in this version, to keep things clear.
This so that OS/2er's can follow whats going on easily. I may release newer
versions with more functions, ie 3D, clipping etc, if people are interested.

Hopefully theres enough to get you started. I thought including source,
aswell as a few sample programs, would give you a fair idea of its workings.

Please use this OS/2 code as you see fit. Its SHARE/FREE WARE. If you cant
afford to send a contribution, dont.  Although any money received will be put
back into the project. ie my internet/pizza/compuserve time costs, as does 
ftp'ing the file to ftp-os2. ....blah...blah..supporting OS/2 shareware
developers...blah...blah....blah....(I bet not one of you buggers send me
a cent  :-)

It would also be nice if you'd send me a copy of any cool games/demos
you write.

Remeber dLib is fast(ish), but it cant work miracles. Games can waste
alot of time thru bad algorithm. A fast graphics library isn't everything.
Slow video card/computer/large view window, can also help cause shocking
performance.

Oh, anybody got any job openings? I'm currently working as a Software Engineer
in New Zealand, but would like to relocate to the US.( :-) its worth a shot)
Resume/code samples/demos available on request. NZ is an OS/2 dead end.....


Any comments? Question?

I can be contacted at,
   Graeme Blackley
   32/2 Webb street
   Wellington
   New Zealand
   ph (NZ) 4382 8119
or,
   (internet)g_blackl@kai.ee.cit.ac.nz
or,
   in fidonet OS2PROG international area.


