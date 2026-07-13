// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//	DOOM graphics stuff for X11, UNIX.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: i_x.c,v 1.6 1997/02/03 22:45:10 b1 Exp $";

#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <X11/extensions/XShm.h>
// Had to dig up XShm.c for this one.
// It is in the libXext, but not in the XFree86 headers.
#ifdef LINUX
int XShmGetEventBase( Display* dpy ); // problems with g++?
#endif

#include <stdarg.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <errno.h>
#include <signal.h>

#include "doomstat.h"
#include "i_system.h"
#include "v_video.h"
#include "m_argv.h"
#include "d_main.h"

#include "doomdef.h"

#define POINTER_WARP_COUNTDOWN	1

Display*	X_display=0;
Window		X_mainWindow;
Colormap	X_cmap;
Visual*		X_visual;
GC		X_gc;
XEvent		X_event;
int		X_screen;
XVisualInfo	X_visualinfo;
XImage*		image;
int		X_width;
int		X_height;

// True when running on a TrueColor (24/32-bit) visual instead of the
// original 256-color PseudoColor one. Modern X servers (XQuartz included)
// essentially never offer an 8-bit PseudoColor visual anymore.
boolean		trueColor;
static unsigned int	truecolorLUT[256];

// MIT SHared Memory extension.
boolean		doShm;

XShmSegmentInfo	X_shminfo;
int		X_shmeventtype;

// Fake mouse handling.
// This cannot work properly w/o DGA.
// Needs an invisible mouse cursor at least.
boolean		grabMouse;
int		doPointerWarp = POINTER_WARP_COUNTDOWN;

// Blocky mode,
// replace each 320x200 pixel with multiply*multiply pixels.
// According to Dave Taylor, it still is a bonehead thing
// to use ....
static int	multiply=1;

static void I_CreateImage(void);
static void I_DestroyImage(void);
static void I_ResizeGraphics(int neww, int newh);

//
//  Translates the key currently in X_event
//

int xlatekey(void)
{

    int rc;

    switch(rc = XKeycodeToKeysym(X_display, X_event.xkey.keycode, 0))
    {
      case XK_Left:	rc = KEY_LEFTARROW;	break;
      case XK_Right:	rc = KEY_RIGHTARROW;	break;
      case XK_Down:	rc = KEY_DOWNARROW;	break;
      case XK_Up:	rc = KEY_UPARROW;	break;
      case XK_Escape:	rc = KEY_ESCAPE;	break;
      case XK_Return:	rc = KEY_ENTER;		break;
      case XK_Tab:	rc = KEY_TAB;		break;
      case XK_F1:	rc = KEY_F1;		break;
      case XK_F2:	rc = KEY_F2;		break;
      case XK_F3:	rc = KEY_F3;		break;
      case XK_F4:	rc = KEY_F4;		break;
      case XK_F5:	rc = KEY_F5;		break;
      case XK_F6:	rc = KEY_F6;		break;
      case XK_F7:	rc = KEY_F7;		break;
      case XK_F8:	rc = KEY_F8;		break;
      case XK_F9:	rc = KEY_F9;		break;
      case XK_F10:	rc = KEY_F10;		break;
      case XK_F11:	rc = KEY_F11;		break;
      case XK_F12:	rc = KEY_F12;		break;
	
      case XK_BackSpace:
      case XK_Delete:	rc = KEY_BACKSPACE;	break;

      case XK_Pause:	rc = KEY_PAUSE;		break;

      case XK_KP_Equal:
      case XK_equal:	rc = KEY_EQUALS;	break;

      case XK_KP_Subtract:
      case XK_minus:	rc = KEY_MINUS;		break;

      case XK_Shift_L:
      case XK_Shift_R:
	rc = KEY_RSHIFT;
	break;
	
      case XK_Control_L:
      case XK_Control_R:
	rc = KEY_RCTRL;
	break;
	
      case XK_Alt_L:
      case XK_Meta_L:
      case XK_Alt_R:
      case XK_Meta_R:
	rc = KEY_RALT;
	break;
	
      default:
	if (rc >= XK_space && rc <= XK_asciitilde)
	    rc = rc - XK_space + ' ';
	if (rc >= 'A' && rc <= 'Z')
	    rc = rc - 'A' + 'a';
	break;
    }

    return rc;

}

void I_ShutdownGraphics(void)
{
  // I_Error() calls this unconditionally, including when I_InitGraphics()
  // never got far enough to open a display or create an image (e.g. no X
  // server running) -- nothing to tear down in that case, and calling into
  // Xlib with a NULL display crashes.
  if (!X_display || !image)
      return;

  I_DestroyImage();
}



//
// I_StartFrame
//
void I_StartFrame (void)
{
    // er?

}

static int	lastmousex = 0;
static int	lastmousey = 0;
boolean		mousemoved = false;
boolean		shmFinished;

void I_GetEvent(void)
{

    event_t event;

    // put event-grabbing stuff in here
    XNextEvent(X_display, &X_event);
    switch (X_event.type)
    {
      case KeyPress:
	event.type = ev_keydown;
	event.data1 = xlatekey();
	D_PostEvent(&event);
	// fprintf(stderr, "k");
	break;
      case KeyRelease:
	event.type = ev_keyup;
	event.data1 = xlatekey();
	D_PostEvent(&event);
	// fprintf(stderr, "ku");
	break;
      case ButtonPress:
	event.type = ev_mouse;
	event.data1 =
	    (X_event.xbutton.state & Button1Mask)
	    | (X_event.xbutton.state & Button2Mask ? 2 : 0)
	    | (X_event.xbutton.state & Button3Mask ? 4 : 0)
	    | (X_event.xbutton.button == Button1)
	    | (X_event.xbutton.button == Button2 ? 2 : 0)
	    | (X_event.xbutton.button == Button3 ? 4 : 0);
	event.data2 = event.data3 = 0;
	D_PostEvent(&event);
	// fprintf(stderr, "b");
	break;
      case ButtonRelease:
	event.type = ev_mouse;
	event.data1 =
	    (X_event.xbutton.state & Button1Mask)
	    | (X_event.xbutton.state & Button2Mask ? 2 : 0)
	    | (X_event.xbutton.state & Button3Mask ? 4 : 0);
	// suggest parentheses around arithmetic in operand of |
	event.data1 =
	    event.data1
	    ^ (X_event.xbutton.button == Button1 ? 1 : 0)
	    ^ (X_event.xbutton.button == Button2 ? 2 : 0)
	    ^ (X_event.xbutton.button == Button3 ? 4 : 0);
	event.data2 = event.data3 = 0;
	D_PostEvent(&event);
	// fprintf(stderr, "bu");
	break;
      case MotionNotify:
	event.type = ev_mouse;
	event.data1 =
	    (X_event.xmotion.state & Button1Mask)
	    | (X_event.xmotion.state & Button2Mask ? 2 : 0)
	    | (X_event.xmotion.state & Button3Mask ? 4 : 0);
	event.data2 = (X_event.xmotion.x - lastmousex) << 2;
	event.data3 = (lastmousey - X_event.xmotion.y) << 2;

	if (event.data2 || event.data3)
	{
	    lastmousex = X_event.xmotion.x;
	    lastmousey = X_event.xmotion.y;
	    if (X_event.xmotion.x != X_width/2 &&
		X_event.xmotion.y != X_height/2)
	    {
		D_PostEvent(&event);
		// fprintf(stderr, "m");
		mousemoved = false;
	    } else
	    {
		mousemoved = true;
	    }
	}
	break;
	
      case Expose:
	break;

      case ConfigureNotify:
	// Window was resized (drag, or the native macOS zoom/maximize
	// button) -- reallocate the image to match.
	I_ResizeGraphics(X_event.xconfigure.width, X_event.xconfigure.height);
	break;

      default:
	if (doShm && X_event.type == X_shmeventtype) shmFinished = true;
	break;
    }

}

Cursor
createnullcursor
( Display*	display,
  Window	root )
{
    Pixmap cursormask;
    XGCValues xgc;
    GC gc;
    XColor dummycolour;
    Cursor cursor;

    cursormask = XCreatePixmap(display, root, 1, 1, 1/*depth*/);
    xgc.function = GXclear;
    gc =  XCreateGC(display, cursormask, GCFunction, &xgc);
    XFillRectangle(display, cursormask, gc, 0, 0, 1, 1);
    dummycolour.pixel = 0;
    dummycolour.red = 0;
    dummycolour.flags = 04;
    cursor = XCreatePixmapCursor(display, cursormask, cursormask,
				 &dummycolour,&dummycolour, 0,0);
    XFreePixmap(display,cursormask);
    XFreeGC(display,gc);
    return cursor;
}

//
// I_StartTic
//
void I_StartTic (void)
{

    if (!X_display)
	return;

    while (XPending(X_display))
	I_GetEvent();

    // Warp the pointer back to the middle of the window
    //  or it will wander off - that is, the game will
    //  loose input focus within X11.
    if (grabMouse)
    {
	if (!--doPointerWarp)
	{
	    XWarpPointer( X_display,
			  None,
			  X_mainWindow,
			  0, 0,
			  0, 0,
			  X_width/2, X_height/2);

	    doPointerWarp = POINTER_WARP_COUNTDOWN;
	}
    }

    mousemoved = false;

}


//
// I_UpdateNoBlit
//
void I_UpdateNoBlit (void)
{
    // what is this?
}

//
// I_FinishUpdate
//
void I_FinishUpdate (void)
{

    static int	lasttic;
    int		tics;
    int		i;
    // UNUSED static unsigned char *bigscreen=0;

    // draws little dots on the bottom of the screen
    if (devparm)
    {

	i = I_GetTime();
	tics = i - lasttic;
	lasttic = i;
	if (tics > 20) tics = 20;

	for (i=0 ; i<tics*2 ; i+=2)
	    screens[0][ (SCREENHEIGHT-1)*SCREENWIDTH + i] = 0xff;
	for ( ; i<20*2 ; i+=2)
	    screens[0][ (SCREENHEIGHT-1)*SCREENWIDTH + i] = 0x0;
    
    }

    // scales the screen size before blitting it
    if (trueColor)
    {
	// Expand palette indices to real RGB pixels (via truecolorLUT),
	// scaling to fit the window's current size -- one nearest-neighbor
	// source lookup per destination pixel. Handles the initial
	// multiply-based size and any later live resize with the same
	// simple loop. This replaces the PseudoColor-only byte-packing
	// tricks below, which assumed a pixel format that doesn't apply
	// here.
	//
	// The 320x200 aspect ratio is preserved (never stretched out of
	// shape): scale to fit inside the window and letterbox/pillarbox
	// the rest in black, centered.
	byte*		src = screens[0];
	unsigned int*	dst;
	int		x, y, srcx, srcy;
	byte*		srcrow;
	int		destW, destH, offX, offY;

	if (X_width * SCREENHEIGHT > X_height * SCREENWIDTH)
	{
	    destH = X_height;
	    destW = X_height * SCREENWIDTH / SCREENHEIGHT;
	}
	else
	{
	    destW = X_width;
	    destH = X_width * SCREENHEIGHT / SCREENWIDTH;
	}
	offX = (X_width - destW) / 2;
	offY = (X_height - destH) / 2;

	if (destW != X_width || destH != X_height)
	    memset(image->data, 0, (size_t)X_width * X_height * 4);

	for (y=0 ; y<destH ; y++)
	{
	    srcy = (y * SCREENHEIGHT) / destH;
	    srcrow = src + srcy * SCREENWIDTH;
	    dst = (unsigned int*) image->data + (offY + y) * X_width + offX;
	    for (x=0 ; x<destW ; x++)
	    {
		srcx = (x * SCREENWIDTH) / destW;
		*dst++ = truecolorLUT[srcrow[srcx]];
	    }
	}
    }
    else if (multiply == 2)
    {
	unsigned int *olineptrs[2];
	unsigned int *ilineptr;
	int x, y, i;
	unsigned int twoopixels;
	unsigned int twomoreopixels;
	unsigned int fouripixels;

	ilineptr = (unsigned int *) (screens[0]);
	for (i=0 ; i<2 ; i++)
	    olineptrs[i] = (unsigned int *) &image->data[i*X_width];

	y = SCREENHEIGHT;
	while (y--)
	{
	    x = SCREENWIDTH;
	    do
	    {
		fouripixels = *ilineptr++;
		twoopixels =	(fouripixels & 0xff000000)
		    |	((fouripixels>>8) & 0xffff00)
		    |	((fouripixels>>16) & 0xff);
		twomoreopixels =	((fouripixels<<16) & 0xff000000)
		    |	((fouripixels<<8) & 0xffff00)
		    |	(fouripixels & 0xff);
#ifdef __BIG_ENDIAN__
		*olineptrs[0]++ = twoopixels;
		*olineptrs[1]++ = twoopixels;
		*olineptrs[0]++ = twomoreopixels;
		*olineptrs[1]++ = twomoreopixels;
#else
		*olineptrs[0]++ = twomoreopixels;
		*olineptrs[1]++ = twomoreopixels;
		*olineptrs[0]++ = twoopixels;
		*olineptrs[1]++ = twoopixels;
#endif
	    } while (x-=4);
	    olineptrs[0] += X_width/4;
	    olineptrs[1] += X_width/4;
	}

    }
    else if (multiply == 3)
    {
	unsigned int *olineptrs[3];
	unsigned int *ilineptr;
	int x, y, i;
	unsigned int fouropixels[3];
	unsigned int fouripixels;

	ilineptr = (unsigned int *) (screens[0]);
	for (i=0 ; i<3 ; i++)
	    olineptrs[i] = (unsigned int *) &image->data[i*X_width];

	y = SCREENHEIGHT;
	while (y--)
	{
	    x = SCREENWIDTH;
	    do
	    {
		fouripixels = *ilineptr++;
		fouropixels[0] = (fouripixels & 0xff000000)
		    |	((fouripixels>>8) & 0xff0000)
		    |	((fouripixels>>16) & 0xffff);
		fouropixels[1] = ((fouripixels<<8) & 0xff000000)
		    |	(fouripixels & 0xffff00)
		    |	((fouripixels>>8) & 0xff);
		fouropixels[2] = ((fouripixels<<16) & 0xffff0000)
		    |	((fouripixels<<8) & 0xff00)
		    |	(fouripixels & 0xff);
#ifdef __BIG_ENDIAN__
		*olineptrs[0]++ = fouropixels[0];
		*olineptrs[1]++ = fouropixels[0];
		*olineptrs[2]++ = fouropixels[0];
		*olineptrs[0]++ = fouropixels[1];
		*olineptrs[1]++ = fouropixels[1];
		*olineptrs[2]++ = fouropixels[1];
		*olineptrs[0]++ = fouropixels[2];
		*olineptrs[1]++ = fouropixels[2];
		*olineptrs[2]++ = fouropixels[2];
#else
		*olineptrs[0]++ = fouropixels[2];
		*olineptrs[1]++ = fouropixels[2];
		*olineptrs[2]++ = fouropixels[2];
		*olineptrs[0]++ = fouropixels[1];
		*olineptrs[1]++ = fouropixels[1];
		*olineptrs[2]++ = fouropixels[1];
		*olineptrs[0]++ = fouropixels[0];
		*olineptrs[1]++ = fouropixels[0];
		*olineptrs[2]++ = fouropixels[0];
#endif
	    } while (x-=4);
	    olineptrs[0] += 2*X_width/4;
	    olineptrs[1] += 2*X_width/4;
	    olineptrs[2] += 2*X_width/4;
	}

    }
    else if (multiply == 4)
    {
	// Broken. Gotta fix this some day.
	void Expand4(unsigned *, double *);
  	Expand4 ((unsigned *)(screens[0]), (double *) (image->data));
    }

    if (doShm)
    {

	if (!XShmPutImage(	X_display,
				X_mainWindow,
				X_gc,
				image,
				0, 0,
				0, 0,
				X_width, X_height,
				True ))
	    I_Error("XShmPutImage() failed\n");

	// wait for it to finish and processes all input events
	shmFinished = false;
	do
	{
	    I_GetEvent();
	} while (!shmFinished);

    }
    else
    {

	// draw the image
	XPutImage(	X_display,
			X_mainWindow,
			X_gc,
			image,
			0, 0,
			0, 0,
			X_width, X_height );

	// sync up with server
	XSync(X_display, False);

    }

}


//
// I_ReadScreen
//
void I_ReadScreen (byte* scr)
{
    memcpy (scr, screens[0], SCREENWIDTH*SCREENHEIGHT);
}


//
// Palette stuff.
//
static XColor	colors[256];

// Number of trailing zero bits in a color mask (e.g. 0xff0000 -> 16).
static int maskShift(unsigned long mask)
{
    int shift = 0;
    if (!mask)
	return 0;
    while (!(mask & 1))
    {
	mask >>= 1;
	shift++;
    }
    return shift;
}

// Number of set bits in a right-aligned color mask (e.g. 0xff -> 8).
static int maskBits(unsigned long mask)
{
    int bits = 0;
    while (mask & 1)
    {
	mask >>= 1;
	bits++;
    }
    return bits;
}

void UploadNewPalette(Colormap cmap, byte *palette)
{

    register int	i;
    register int	c;
    static boolean	firstcall = true;

#ifdef __cplusplus
    if (X_visualinfo.c_class == PseudoColor && X_visualinfo.depth == 8)
#else
    if (X_visualinfo.class == PseudoColor && X_visualinfo.depth == 8)
#endif
	{
	    // initialize the colormap
	    if (firstcall)
	    {
		firstcall = false;
		for (i=0 ; i<256 ; i++)
		{
		    colors[i].pixel = i;
		    colors[i].flags = DoRed|DoGreen|DoBlue;
		}
	    }

	    // set the X colormap entries
	    for (i=0 ; i<256 ; i++)
	    {
		c = gammatable[usegamma][*palette++];
		colors[i].red = (c<<8) + c;
		c = gammatable[usegamma][*palette++];
		colors[i].green = (c<<8) + c;
		c = gammatable[usegamma][*palette++];
		colors[i].blue = (c<<8) + c;
	    }

	    // store the colors to the current colormap
	    XStoreColors(X_display, cmap, colors, 256);

	}
    else if (trueColor)
	{
	    // Build a index->pixel lookup table using the visual's real
	    // channel masks, so this works regardless of RGB/BGR ordering
	    // or exact bit layout (24 vs 32 bpp).
	    static int	rShift, gShift, bShift;
	    static int	rBits, gBits, bBits;

	    if (firstcall)
	    {
		firstcall = false;
		rShift = maskShift(X_visualinfo.red_mask);
		gShift = maskShift(X_visualinfo.green_mask);
		bShift = maskShift(X_visualinfo.blue_mask);
		rBits = maskBits(X_visualinfo.red_mask >> rShift);
		gBits = maskBits(X_visualinfo.green_mask >> gShift);
		bBits = maskBits(X_visualinfo.blue_mask >> bShift);
	    }

	    for (i=0 ; i<256 ; i++)
	    {
		unsigned int r = gammatable[usegamma][*palette++];
		unsigned int g = gammatable[usegamma][*palette++];
		unsigned int b = gammatable[usegamma][*palette++];

		truecolorLUT[i] =
		    ((r >> (8 - rBits)) << rShift)
		    | ((g >> (8 - gBits)) << gShift)
		    | ((b >> (8 - bBits)) << bShift);
	    }
	}
}

//
// I_SetPalette
//
void I_SetPalette (byte* palette)
{
    UploadNewPalette(X_cmap, palette);
}


//
// This function is probably redundant,
//  if XShmDetach works properly.
// ddt never detached the XShm memory,
//  thus there might have been stale
//  handles accumulating.
//
// Returns false (instead of I_Error'ing) if shared memory couldn't be
// allocated. macOS's default SysV shm limits (kern.sysv.shmall/shmmax)
// are small enough that this is a routine, expected failure here -- not
// a fatal one -- so the caller can fall back to plain Xlib image transfer.
boolean grabsharedmemory(int size)
{

  int			key = ('d'<<24) | ('o'<<16) | ('o'<<8) | 'm';
  struct shmid_ds	shminfo;
  int			minsize = 320*200;
  int			id;
  int			rc;
  // UNUSED int done=0;
  int			pollution=5;

  // try to use what was here before
  do
  {
    id = shmget((key_t) key, minsize, 0777); // just get the id
    if (id != -1)
    {
      rc=shmctl(id, IPC_STAT, &shminfo); // get stats on it
      if (!rc)
      {
	if (shminfo.shm_nattch)
	{
	  fprintf(stderr, "User %d appears to be running "
		  "DOOM.  Is that wise?\n", shminfo.shm_cpid);
	  key++;
	}
	else
	{
	  if (getuid() == shminfo.shm_perm.cuid)
	  {
	    rc = shmctl(id, IPC_RMID, 0);
	    if (!rc)
	      fprintf(stderr,
		      "Was able to kill my old shared memory\n");
	    else
	    {
	      fprintf(stderr, "Was NOT able to kill my old shared memory\n");
	      return false;
	    }

	    id = shmget((key_t)key, size, IPC_CREAT|0777);
	    if (id==-1)
	    {
	      fprintf(stderr, "Could not get shared memory\n");
	      return false;
	    }

	    rc=shmctl(id, IPC_STAT, &shminfo);

	    break;

	  }
	  if (size >= shminfo.shm_segsz)
	  {
	    fprintf(stderr,
		    "will use %d's stale shared memory\n",
		    shminfo.shm_cpid);
	    break;
	  }
	  else
	  {
	    fprintf(stderr,
		    "warning: can't use stale "
		    "shared memory belonging to id %d, "
		    "key=0x%x\n",
		    shminfo.shm_cpid, key);
	    key++;
	  }
	}
      }
      else
      {
	fprintf(stderr, "could not get stats on key=%d\n", key);
	return false;
      }
    }
    else
    {
      id = shmget((key_t)key, size, IPC_CREAT|0777);
      if (id==-1)
      {
	extern int errno;
	fprintf(stderr, "shmget errno=%d\n", errno);
	return false;
      }
      break;
    }
  } while (--pollution);

  if (!pollution)
  {
    fprintf(stderr, "Sorry, system too polluted with stale "
		     "shared memory segments.\n");
    return false;
  }

  X_shminfo.shmid = id;

  // attach to the shared memory segment
  image->data = X_shminfo.shmaddr = shmat(id, 0, 0);

  // shmat() signals failure by returning (void*)-1, NOT NULL -- a
  // pre-existing bug here checked for NULL, which shmat never returns.
  if (image->data == (void*)-1)
  {
    fprintf(stderr, "shmat() failed\n");
    shmctl(id, IPC_RMID, 0);
    image->data = NULL;
    return false;
  }

  fprintf(stderr, "shared memory id=%d, addr=0x%x\n", id,
	  (int) (image->data));

  return true;
}

// Creates `image` (and attaches shared memory, if used) at the current
// X_width/X_height. Used both for the initial window and to reallocate
// on resize. macOS's default SysV shm limits (kern.sysv.shmall/shmmax)
// are small (4MB total, system-wide, by default) and easy for a
// Retina-resolution window to exceed -- grabsharedmemory() reports that
// as a normal failure (returns false) rather than I_Error'ing, so we can
// fall back to plain Xlib image transfer instead of crashing/exiting.
static void I_CreateImage(void)
{
    if (doShm)
    {
	X_shmeventtype = XShmGetEventBase(X_display) + ShmCompletion;

	image = XShmCreateImage(	X_display,
					X_visual,
					X_visualinfo.depth,
					ZPixmap,
					0,
					&X_shminfo,
					X_width,
					X_height );

	if (!image)
	    I_Error("XShmCreateImage() failed in I_CreateImage()");

	if (!grabsharedmemory(image->bytes_per_line * image->height))
	{
	    fprintf(stderr,
		    "Could not allocate System V shared memory for a "
		    "%dx%d window -- falling back to plain Xlib image "
		    "transfer.\n", X_width, X_height);
	    doShm = false;
	}
	else if (!XShmAttach(X_display, &X_shminfo))
	{
	    I_Error("XShmAttach() failed in I_CreateImage()");
	}
    }

    if (!doShm)
    {
	image = XCreateImage(	X_display,
				X_visual,
				X_visualinfo.depth,
				ZPixmap,
				0,
				(char*)malloc(X_width * X_height
					      * (trueColor ? 4 : 1)),
				X_width, X_height,
				trueColor ? 32 : 8,
				0 );

	if (!image)
	    I_Error("XCreateImage() failed in I_CreateImage()");
    }

    if (trueColor && image->bits_per_pixel != 32)
	I_Error("Unsupported TrueColor pixel format (%d bits per pixel)",
		image->bits_per_pixel);
}

// Tears down whatever I_CreateImage() set up, without touching X_display
// or the window -- used both at shutdown and before reallocating on resize.
static void I_DestroyImage(void)
{
    if (doShm)
    {
	// Deliberately not calling XDestroyImage() here: Xlib/XShm's cleanup
	// path for shm-backed images isn't well-documented and risks a
	// double shmdt/free. The small XImage struct this leaks per resize
	// is a non-issue for a study binary.
	if (!XShmDetach(X_display, &X_shminfo))
	    I_Error("XShmDetach() failed in I_DestroyImage()");
	shmdt(X_shminfo.shmaddr);
	shmctl(X_shminfo.shmid, IPC_RMID, 0);
	image->data = NULL;
    }
    else
    {
	XDestroyImage(image);
    }
}

// Called from I_GetEvent() on ConfigureNotify (window resized, e.g. the
// user dragged a corner or clicked the native macOS zoom/maximize button).
// Reallocates `image` at the new size; screens[0] (the 320x200 paletted
// framebuffer) never changes size, only how it gets scaled up in
// I_FinishUpdate.
static void I_ResizeGraphics(int neww, int newh)
{
    if (neww <= 0 || newh <= 0)
	return;
    if (neww == X_width && newh == X_height)
	return;
    // The PseudoColor byte-packing paths (multiply==2/3/4) and the
    // screens[0]==image->data alias (multiply==1) assume a fixed size
    // tied to `multiply` -- arbitrary live resize is TrueColor-only.
    if (!trueColor)
	return;

    I_DestroyImage();
    X_width = neww;
    X_height = newh;
    I_CreateImage();

    // Force the server to drop whatever it had displayed at the old size
    // and repaint the whole (new-size) window from scratch on the next
    // I_FinishUpdate -- otherwise a stale frame can be left on screen
    // after an animated resize/zoom until something else triggers a
    // redraw.
    XClearArea(X_display, X_mainWindow, 0, 0, 0, 0, False);
    XSync(X_display, False);
}

void I_InitGraphics(void)
{

    char*		displayname;
    char*		d;
    int			n;
    int			pnum;
    int			x=0;
    int			y=0;
    
    // warning: char format, different type arg
    char		xsign=' ';
    char		ysign=' ';
    
    int			oktodraw;
    unsigned long	attribmask;
    XSetWindowAttributes attribs;
    XGCValues		xgcvalues;
    int			valuemask;
    static int		firsttime=1;

    if (!firsttime)
	return;
    firsttime = 0;

    signal(SIGINT, (void (*)(int)) I_Quit);

    if (M_CheckParm("-2"))
	multiply = 2;

    if (M_CheckParm("-3"))
	multiply = 3;

    if (M_CheckParm("-4"))
	multiply = 4;

    X_width = SCREENWIDTH * multiply;
    X_height = SCREENHEIGHT * multiply;

    // check for command-line display name
    if ( (pnum=M_CheckParm("-disp")) ) // suggest parentheses around assignment
	displayname = myargv[pnum+1];
    else
	displayname = 0;

    // check if the user wants to grab the mouse (quite unnice)
    grabMouse = !!M_CheckParm("-grabmouse");

    // check for command-line geometry
    if ( (pnum=M_CheckParm("-geom")) ) // suggest parentheses around assignment
    {
	// warning: char format, different type arg 3,5
	n = sscanf(myargv[pnum+1], "%c%d%c%d", &xsign, &x, &ysign, &y);
	
	if (n==2)
	    x = y = 0;
	else if (n==6)
	{
	    if (xsign == '-')
		x = -x;
	    if (ysign == '-')
		y = -y;
	}
	else
	    I_Error("bad -geom parameter");
    }

    // open the display
    X_display = XOpenDisplay(displayname);
    if (!X_display)
    {
	if (displayname)
	    I_Error("Could not open display [%s]", displayname);
	else
	    I_Error("Could not open display (DISPLAY=[%s])", getenv("DISPLAY"));
    }

    // use the default visual
    X_screen = DefaultScreen(X_display);
    if (XMatchVisualInfo(X_display, X_screen, 8, PseudoColor, &X_visualinfo))
    {
	trueColor = false;
    }
    else if (XMatchVisualInfo(X_display, X_screen, 32, TrueColor, &X_visualinfo)
	     || XMatchVisualInfo(X_display, X_screen, 24, TrueColor, &X_visualinfo))
    {
	// multiply (-2/-3/-4) is handled generically for TrueColor by
	// I_FinishUpdate's index->RGB expansion loop (nearest-neighbor
	// upscale), unlike the PseudoColor byte-packing tricks below.
	trueColor = true;
    }
    else
    {
	I_Error("xdoom requires either a 256-color PseudoColor screen "
		"or a 24/32-bit TrueColor screen");
    }
    X_visual = X_visualinfo.visual;

    // check for the MITSHM extension
    doShm = XShmQueryExtension(X_display);

    // even if it's available, make sure it's a local connection
    if (doShm)
    {
	if (!displayname) displayname = (char *) getenv("DISPLAY");
	if (displayname)
	{
	    d = displayname;
	    while (*d && (*d != ':')) d++;
	    if (*d) *d = 0;
	    if (!(!strcasecmp(displayname, "unix") || !*displayname)) doShm = false;
	}
    }

    fprintf(stderr, "Using MITSHM extension\n");

    // create the colormap
    // TrueColor visuals are read-only; AllocAll is only valid for
    // PseudoColor, where we own and rewrite the whole palette ourselves.
    X_cmap = XCreateColormap(X_display, RootWindow(X_display,
						   X_screen), X_visual,
			      trueColor ? AllocNone : AllocAll);

    // setup attributes for main window
    attribmask = CWEventMask | CWColormap | CWBorderPixel;
    attribs.event_mask =
	KeyPressMask
	| KeyReleaseMask
	// | PointerMotionMask | ButtonPressMask | ButtonReleaseMask
	| StructureNotifyMask	// so we get ConfigureNotify on resize
	| ExposureMask;

    attribs.colormap = X_cmap;
    attribs.border_pixel = 0;

    if (trueColor)
    {
	// Any part of the window not covered by our (letterboxed, centered)
	// image -- e.g. briefly during a resize/zoom animation -- shows
	// this instead of an undefined/white default.
	attribmask |= CWBackPixel;
	attribs.background_pixel = BlackPixel(X_display, X_screen);
    }

    // create the main window
    X_mainWindow = XCreateWindow(	X_display,
					RootWindow(X_display, X_screen),
					x, y,
					X_width, X_height,
					0, // borderwidth
					X_visualinfo.depth, // depth
					InputOutput,
					X_visual,
					attribmask,
					&attribs );

    XDefineCursor(X_display, X_mainWindow,
		  createnullcursor( X_display, X_mainWindow ) );

    // create the GC
    valuemask = GCGraphicsExposures;
    xgcvalues.graphics_exposures = False;
    X_gc = XCreateGC(	X_display,
  			X_mainWindow,
  			valuemask,
  			&xgcvalues );

    // map the window
    XMapWindow(X_display, X_mainWindow);

    // wait until it is OK to draw
    oktodraw = 0;
    while (!oktodraw)
    {
	XNextEvent(X_display, &X_event);
	if (X_event.type == Expose
	    && !X_event.xexpose.count)
	{
	    oktodraw = 1;
	}
    }

    // grabs the pointer so it is restricted to this window
    if (grabMouse)
	XGrabPointer(X_display, X_mainWindow, True,
		     ButtonPressMask|ButtonReleaseMask|PointerMotionMask,
		     GrabModeAsync, GrabModeAsync,
		     X_mainWindow, None, CurrentTime);

    I_CreateImage();

    // With a TrueColor display, image->data holds real RGB pixels, so the
    // engine's 8-bit paletted framebuffer has to live in its own buffer
    // and get expanded through truecolorLUT every frame (see
    // I_FinishUpdate). With PseudoColor, the engine can keep writing
    // palette indices directly into what the X server displays. Either
    // way this is sized for the native 320x200 buffer only, and doesn't
    // need to change on a later resize.
    if (!trueColor && multiply == 1)
	screens[0] = (unsigned char *) (image->data);
    else
	screens[0] = (unsigned char *) malloc (SCREENWIDTH * SCREENHEIGHT);
}


unsigned	exptable[256];

void InitExpand (void)
{
    int		i;
	
    for (i=0 ; i<256 ; i++)
	exptable[i] = i | (i<<8) | (i<<16) | (i<<24);
}

double		exptable2[256*256];

void InitExpand2 (void)
{
    int		i;
    int		j;
    // UNUSED unsigned	iexp, jexp;
    double*	exp;
    union
    {
	double 		d;
	unsigned	u[2];
    } pixel;
	
    printf ("building exptable2...\n");
    exp = exptable2;
    for (i=0 ; i<256 ; i++)
    {
	pixel.u[0] = i | (i<<8) | (i<<16) | (i<<24);
	for (j=0 ; j<256 ; j++)
	{
	    pixel.u[1] = j | (j<<8) | (j<<16) | (j<<24);
	    *exp++ = pixel.d;
	}
    }
    printf ("done.\n");
}

int	inited;

void
Expand4
( unsigned*	lineptr,
  double*	xline )
{
    double	dpixel;
    unsigned	x;
    unsigned 	y;
    unsigned	fourpixels;
    unsigned	step;
    double*	exp;
	
    exp = exptable2;
    if (!inited)
    {
	inited = 1;
	InitExpand2 ();
    }
		
		
    step = 3*SCREENWIDTH/2;
	
    y = SCREENHEIGHT-1;
    do
    {
	x = SCREENWIDTH;

	do
	{
	    fourpixels = lineptr[0];
			
	    dpixel = *(double *)( (int)exp + ( (fourpixels&0xffff0000)>>13) );
	    xline[0] = dpixel;
	    xline[160] = dpixel;
	    xline[320] = dpixel;
	    xline[480] = dpixel;
			
	    dpixel = *(double *)( (int)exp + ( (fourpixels&0xffff)<<3 ) );
	    xline[1] = dpixel;
	    xline[161] = dpixel;
	    xline[321] = dpixel;
	    xline[481] = dpixel;

	    fourpixels = lineptr[1];
			
	    dpixel = *(double *)( (int)exp + ( (fourpixels&0xffff0000)>>13) );
	    xline[2] = dpixel;
	    xline[162] = dpixel;
	    xline[322] = dpixel;
	    xline[482] = dpixel;
			
	    dpixel = *(double *)( (int)exp + ( (fourpixels&0xffff)<<3 ) );
	    xline[3] = dpixel;
	    xline[163] = dpixel;
	    xline[323] = dpixel;
	    xline[483] = dpixel;

	    fourpixels = lineptr[2];
			
	    dpixel = *(double *)( (int)exp + ( (fourpixels&0xffff0000)>>13) );
	    xline[4] = dpixel;
	    xline[164] = dpixel;
	    xline[324] = dpixel;
	    xline[484] = dpixel;
			
	    dpixel = *(double *)( (int)exp + ( (fourpixels&0xffff)<<3 ) );
	    xline[5] = dpixel;
	    xline[165] = dpixel;
	    xline[325] = dpixel;
	    xline[485] = dpixel;

	    fourpixels = lineptr[3];
			
	    dpixel = *(double *)( (int)exp + ( (fourpixels&0xffff0000)>>13) );
	    xline[6] = dpixel;
	    xline[166] = dpixel;
	    xline[326] = dpixel;
	    xline[486] = dpixel;
			
	    dpixel = *(double *)( (int)exp + ( (fourpixels&0xffff)<<3 ) );
	    xline[7] = dpixel;
	    xline[167] = dpixel;
	    xline[327] = dpixel;
	    xline[487] = dpixel;

	    lineptr+=4;
	    xline+=8;
	} while (x-=16);
	xline += step;
    } while (y--);
}


