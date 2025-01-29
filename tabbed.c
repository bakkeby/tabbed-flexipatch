/*
 * See LICENSE file for copyright and license details.
 */

#include <sys/wait.h>
#include <locale.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <X11/Xft/Xft.h>

#include "patches.h"
#include "arg.h"

/* XEMBED messages */
#define XEMBED_EMBEDDED_NOTIFY          0
#define XEMBED_WINDOW_ACTIVATE          1
#define XEMBED_WINDOW_DEACTIVATE        2
#define XEMBED_REQUEST_FOCUS            3
#define XEMBED_FOCUS_IN                 4
#define XEMBED_FOCUS_OUT                5
#define XEMBED_FOCUS_NEXT               6
#define XEMBED_FOCUS_PREV               7
/* 8-9 were used for XEMBED_GRAB_KEY/XEMBED_UNGRAB_KEY */
#define XEMBED_MODALITY_ON              10
#define XEMBED_MODALITY_OFF             11
#define XEMBED_REGISTER_ACCELERATOR     12
#define XEMBED_UNREGISTER_ACCELERATOR   13
#define XEMBED_ACTIVATE_ACCELERATOR     14

/* Details for  XEMBED_FOCUS_IN: */
#define XEMBED_FOCUS_CURRENT            0
#define XEMBED_FOCUS_FIRST              1
#define XEMBED_FOCUS_LAST               2

/* Macros */
#define MAX(a, b)               ((a) > (b) ? (a) : (b))
#define MIN(a, b)               ((a) < (b) ? (a) : (b))
#define LENGTH(x)               (sizeof((x)) / sizeof(*(x)))
#define CLEANMASK(mask)         (mask & ~(numlockmask | LockMask))
#define TEXTW(x)                (textnw(x, strlen(x)) + dc.font.height)

enum { ColFG, ColBG, ColLast };       /* color */
enum {
	WMProtocols,
	WMDelete,
	WMName,
	WMState,
	WMFullscreen,
	XEmbed,
	WMSelectTab,
	#if ICON_PATCH
	WMIcon,
	#endif // ICON_PATCH
	#if CLASS_PATCH
	WMClass,
	#endif // CLASS_PATCH
	WMLast
}; /* default atoms */

typedef union {
	int i;
	const void *v;
} Arg;

typedef struct {
	unsigned int mod;
	#if KEYCODE_PATCH
	KeyCode keycode;
	#else
	KeySym keysym;
	#endif // KEYCODE_PATCH
	void (*func)(const Arg *);
	const Arg arg;
} Key;

typedef struct {
	int x, y, w, h;
	XftColor norm[ColLast];
	XftColor sel[ColLast];
	XftColor urg[ColLast];
	Drawable drawable;
	GC gc;
	struct {
		int ascent;
		int descent;
		int height;
		XftFont *xfont;
	} font;
} DC; /* draw context */

typedef struct {
	char name[256];
	#if BASENAME_PATCH
	char *basename;
	#endif // BASENAME_PATCH
	Window win;
	int tabx;
	Bool urgent;
	Bool closed;
} Client;

/* function declarations */
static void buttonpress(const XEvent *e);
static void cleanup(void);
static void clientmessage(const XEvent *e);
static void configurenotify(const XEvent *e);
static void configurerequest(const XEvent *e);
static void createnotify(const XEvent *e);
static void destroynotify(const XEvent *e);
static void die(const char *errstr, ...);
static void drawbar(void);
static void drawtext(const char *text, XftColor col[ColLast]);
static void *ecalloc(size_t n, size_t size);
static void *erealloc(void *o, size_t size);
static void expose(const XEvent *e);
static void focus(int c);
static void focusin(const XEvent *e);
static void focusonce(const Arg *arg);
static void focusurgent(const Arg *arg);
static void fullscreen(const Arg *arg);
static char *getatom(int a);
static int getclient(Window w);
static XftColor getcolor(const char *colstr);
static int getfirsttab(void);
static Bool gettextprop(Window w, Atom atom, char *text, unsigned int size);
static void initfont(const char *fontstr);
static Bool isprotodel(int c);
static void keypress(const XEvent *e);
static void killclient(const Arg *arg);
static void manage(Window win);
static void maprequest(const XEvent *e);
static void move(const Arg *arg);
static void movetab(const Arg *arg);
static void propertynotify(const XEvent *e);
static void resize(int c, int w, int h);
static void rotate(const Arg *arg);
static void run(void);
static void sendxembed(int c, long msg, long detail, long d1, long d2);
static void setcmd(int argc, char *argv[], int);
static void setup(void);
static void spawn(const Arg *arg);
static int textnw(const char *text, unsigned int len);
static void toggle(const Arg *arg);
static void unmanage(int c);
static void unmapnotify(const XEvent *e);
static void updatenumlockmask(void);
static void updatetitle(int c);
static int xerror(Display *dpy, XErrorEvent *ee);
static void xsettitle(Window w, const char *str);

#include "patch/include.h"

/* variables */
static int screen;
static void (*handler[LASTEvent]) (const XEvent *) = {
	[ButtonPress] = buttonpress,
	[ClientMessage] = clientmessage,
	[ConfigureNotify] = configurenotify,
	[ConfigureRequest] = configurerequest,
	[CreateNotify] = createnotify,
	[UnmapNotify] = unmapnotify,
	[DestroyNotify] = destroynotify,
	[Expose] = expose,
	[FocusIn] = focusin,
	[KeyPress] = keypress,
	#if KEYRELEASE_PATCH
	[KeyRelease] = keyrelease,
	#endif // KEYRELEASE_PATCH
	[MapRequest] = maprequest,
	[PropertyNotify] = propertynotify,
	#if DRAG_PATCH
	[MotionNotify] = motionnotify,
	#endif // DRAG_PATCH
};
static int bh, obh, wx, wy, ww, wh;
#if AUTOHIDE_PATCH || HIDETABS_PATCH
static int vbh;
#endif // AUTOHIDE_PATCH | HIDETABS_PATCH
static unsigned int numlockmask;
static Bool running = True, nextfocus, doinitspawn = True,
            fillagain = False, closelastclient = False,
            killclientsfirst = False;
static Display *dpy;
static DC dc;
static Atom wmatom[WMLast];
static Window root, win;
static Client **clients;
static int nclients, sel = -1, lastsel = -1;
static int (*xerrorxlib)(Display *, XErrorEvent *);
static int cmd_append_pos;
static char winid[64];
static char **cmd;
static char *wmname = "tabbed";
#if CLASS_PATCH
static char *wmclass= "tabbed";
#endif // CLASS_PATCH
static const char *geometry;
#if HIDETABS_PATCH
static Bool barvisibility = False;
#endif // HIDETABS_PATCH

#if ALPHA_PATCH
static Colormap cmap;
static Visual *visual = NULL;
#endif // ALPHA_PATCH

char *argv0;

/* configuration, allows nested code to access above variables */
#include "config.h"

#include "patch/include.c"

void
buttonpress(const XEvent *e)
{
	const XButtonPressedEvent *ev = &e->xbutton;
	int i, fc;
	Arg arg;

	#if BOTTOM_TABS_PATCH
	if (ev->y < wh - bh)
	#else
	if (ev->y < 0 || ev->y > bh)
	#endif // BOTTOM_TABS_PATCH
		return;

	if (((fc = getfirsttab()) > 0 && ev->x < TEXTW(before)) || ev->x < 0)
		return;

	for (i = fc; i < nclients; i++) {
		if (clients[i]->tabx > ev->x) {
			switch (ev->button) {
			case Button1:
				focus(i);
				break;
			case Button2:
				focus(i);
				killclient(NULL);
				break;
			case Button4: /* FALLTHROUGH */
			case Button5:
				arg.i = ev->button == Button4 ? -1 : 1;
				rotate(&arg);
				break;
			}
			break;
		}
	}
}

void
cleanup(void)
{
	int i;

	for (i = 0; i < nclients; i++) {
		focus(i);
		killclient(NULL);
		XReparentWindow(dpy, clients[i]->win, root, 0, 0);
		unmanage(i);
	}
	free(clients);
	clients = NULL;

	XFreePixmap(dpy, dc.drawable);
	XFreeGC(dpy, dc.gc);
	XDestroyWindow(dpy, win);
	XSync(dpy, False);
	free(cmd);
}

void
clientmessage(const XEvent *e)
{
	const XClientMessageEvent *ev = &e->xclient;

	if (ev->message_type == wmatom[WMProtocols] &&
	    ev->data.l[0] == wmatom[WMDelete]) {
		if (nclients > 1 && killclientsfirst) {
			killclient(0);
			return;
		}
		running = False;
	}
}

void
configurenotify(const XEvent *e)
{
	const XConfigureEvent *ev = &e->xconfigure;

	if (ev->window == win && (ev->width != ww || ev->height != wh)) {
		ww = ev->width;
		wh = ev->height;
		XFreePixmap(dpy, dc.drawable);
		#if ALPHA_PATCH
		dc.drawable = XCreatePixmap(dpy, win, ww, wh,
		              32);
		#else
		dc.drawable = XCreatePixmap(dpy, root, ww, wh,
		              DefaultDepth(dpy, screen));
		#endif // ALPHA_PATCH

		if (!obh && (wh <= bh)) {
			obh = bh;
			bh = 0;
		} else if (!bh && (wh > obh)) {
			bh = obh;
			obh = 0;
		}

		if (sel > -1)
			resize(sel, ww, wh - bh);
		XSync(dpy, False);
	}
}

void
configurerequest(const XEvent *e)
{
	const XConfigureRequestEvent *ev = &e->xconfigurerequest;
	XWindowChanges wc;
	int c;

	if ((c = getclient(ev->window)) > -1) {
		wc.x = 0;
		wc.y = bh;
		wc.width = ww;
		wc.height = wh - bh;
		wc.border_width = 0;
		wc.sibling = ev->above;
		wc.stack_mode = ev->detail;
		XConfigureWindow(dpy, clients[c]->win, ev->value_mask, &wc);
	}
}

void
createnotify(const XEvent *e)
{
	const XCreateWindowEvent *ev = &e->xcreatewindow;

	if (ev->window != win && getclient(ev->window) < 0)
		manage(ev->window);
}

void
destroynotify(const XEvent *e)
{
	const XDestroyWindowEvent *ev = &e->xdestroywindow;
	int c;

	if ((c = getclient(ev->window)) > -1)
		unmanage(c);
}

void
die(const char *errstr, ...)
{
	va_list ap;

	va_start(ap, errstr);
	vfprintf(stderr, errstr, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

void
drawbar(void)
{
	XftColor *col;
	int c, cc, fc, width;
	#if AUTOHIDE_PATCH || HIDETABS_PATCH
	int nbh;
	#endif // AUTOHIDE_PATCH | HIDETABS_PATCH
	char *name = NULL;
	#if CLIENTNUMBER_PATCH
	char tabtitle[312];
	#endif // CLIENTNUMBER_PATCH
	#if BOTTOM_TABS_PATCH
	int by = wh - bh;
	#else
	int by = 0;
	#endif // BOTTOM_TABS_PATCH

	#if XRESOURCES_PATCH && XRESOURCES_RELOAD_PATCH
	if (colors_changed)
		writecolors();
	#endif // XRESOURCES_RELOAD_PATCH

	#if AUTOHIDE_PATCH || HIDETABS_PATCH
	#if AUTOHIDE_PATCH && HIDETABS_PATCH
	nbh = barvisibility && nclients > 1 ? vbh : 0;
	#elif HIDETABS_PATCH
	nbh = barvisibility ? vbh : 0;
	#elif AUTOHIDE_PATCH
	nbh = nclients > 1 ? vbh : 0;
	#endif
	if (nbh != bh) {
		bh = nbh;
		#if BOTTOM_TABS_PATCH
		by = wh - bh;
		#endif // BOTTOM_TABS_PATCH
		for (c = 0; c < nclients; c++)
			#if BOTTOM_TABS_PATCH
			XMoveResizeWindow(dpy, clients[c]->win, 0, 0, ww, wh - bh);
			#else
			XMoveResizeWindow(dpy, clients[c]->win, 0, bh, ww, wh - bh);
			#endif // BOTTOM_TABS_PATCH
	}
	#endif // AUTOHIDE_PATCH | HIDETABS_PATCH

	if (nclients == 0) {
		dc.x = 0;
		dc.w = ww;
		XFetchName(dpy, win, &name);
		drawtext(name ? name : "", dc.norm);
		#if AUTOHIDE_PATCH
		XCopyArea(dpy, dc.drawable, win, dc.gc, 0, 0, ww, vbh, 0, by);
		#else
		XCopyArea(dpy, dc.drawable, win, dc.gc, 0, 0, ww, bh, 0, by);
		#endif // AUTOHIDE_PATCH
		XSync(dpy, False);

		return;
	}

	#if AUTOHIDE_PATCH || HIDETABS_PATCH
	if (bh == 0)
		return;
	#endif // AUTOHIDE_PATCH | HIDETABS_PATCH

	width = ww;

	#if AWESOMEBAR_PATCH
	tabwidth = ww / nclients;
	#endif // AWESOMEBAR_PATCH
	cc = ww / tabwidth;
	if (nclients > cc)
		cc = (ww - TEXTW(before) - TEXTW(after)) / tabwidth;

	if ((fc = getfirsttab()) + cc < nclients) {
		dc.w = TEXTW(after);
		dc.x = width - dc.w;
		drawtext(after, dc.sel);
		width -= dc.w;
	}
	dc.x = 0;

	if (fc > 0) {
		dc.w = TEXTW(before);
		drawtext(before, dc.sel);
		dc.x += dc.w;
		width -= dc.w;
	}

	cc = MIN(cc, nclients);
	for (c = fc; c < fc + cc; c++) {
		dc.w = width / cc;
		if (c == sel) {
			col = dc.sel;
			dc.w += width % cc;
		} else {
			col = clients[c]->urgent ? dc.urg : dc.norm;
		}
		#if CLIENTNUMBER_PATCH && BASENAME_PATCH
		snprintf(tabtitle, sizeof(tabtitle), "%d: %s",
		         c + 1, basenametitles ? clients[c]->basename : clients[c]->name);
		drawtext(tabtitle, col);
		#elif CLIENTNUMBER_PATCH
		snprintf(tabtitle, sizeof(tabtitle), "%d: %s",
		         c + 1, clients[c]->name);
		drawtext(tabtitle, col);
		#elif BASENAME_PATCH
		drawtext(basenametitles ? clients[c]->basename : clients[c]->name, col);
		#else
		drawtext(clients[c]->name, col);
		#endif // CLIENTNUMBER_PATCH
		dc.x += dc.w;
		clients[c]->tabx = dc.x;
	}
	XCopyArea(dpy, dc.drawable, win, dc.gc, 0, 0, ww, bh, 0, by);
	XSync(dpy, False);
}

void
drawtext(const char *text, XftColor col[ColLast])
{
	int i, j, x, y, h, len, olen;
	char buf[256];
	XftDraw *d;
	#if SEPARATOR_PATCH
	XRectangle tab = { dc.x+separator, dc.y, dc.w-separator, dc.h };
	XRectangle sep = { dc.x, dc.y, separator, dc.h };

	if (separator) {
		XSetForeground(dpy, dc.gc, col[ColFG].pixel);
		XFillRectangles(dpy, dc.drawable, dc.gc, &sep, 1);
	}

	XSetForeground(dpy, dc.gc, col[ColBG].pixel);
	XFillRectangles(dpy, dc.drawable, dc.gc, &tab, 1);
	#else
	XRectangle r = { dc.x, dc.y, dc.w, dc.h };
	XSetForeground(dpy, dc.gc, col[ColBG].pixel);
	XFillRectangles(dpy, dc.drawable, dc.gc, &r, 1);
	#endif // SEPARATOR_PATCH

	if (!text)
		return;

	olen = strlen(text);
	h = dc.font.ascent + dc.font.descent;
	y = dc.y + (dc.h / 2) - (h / 2) + dc.font.ascent;
	x = dc.x + (h / 2);

	/* shorten text if necessary */
	for (len = MIN(olen, sizeof(buf));
		len && textnw(text, len) > dc.w - h; len--);

	if (!len)
		return;

	memcpy(buf, text, len);
	if (len < olen) {
		for (i = len, j = strlen(titletrim); j && i; buf[--i] = titletrim[--j]);
	}
	#if CENTER_PATCH
	else
		x += (dc.w - TEXTW(buf)) / 2; // center text
	#endif // CENTER_PATCH

	#if ALPHA_PATCH
	d = XftDrawCreate(dpy, dc.drawable, visual, cmap);
	#else
	d = XftDrawCreate(dpy, dc.drawable, DefaultVisual(dpy, screen), DefaultColormap(dpy, screen));
	#endif // ALPHA_PATCH
	XftDrawStringUtf8(d, &col[ColFG], dc.font.xfont, x, y, (XftChar8 *) buf, len);
	XftDrawDestroy(d);
}

void *
ecalloc(size_t n, size_t size)
{
	void *p;

	if (!(p = calloc(n, size)))
		die("%s: cannot calloc\n", argv0);
	return p;
}

void *
erealloc(void *o, size_t size)
{
	void *p;

	if (!(p = realloc(o, size)))
		die("%s: cannot realloc\n", argv0);
	return p;
}

void
expose(const XEvent *e)
{
	const XExposeEvent *ev = &e->xexpose;

	if (ev->count == 0 && win == ev->window)
		drawbar();
}

void
focus(int c)
{
	char buf[BUFSIZ] = "tabbed-"VERSION" ::";
	size_t i, n;
	XWMHints* wmh;
	XWMHints* win_wmh;

	/* If c, sel and clients are -1, raise tabbed-win itself */
	if (nclients == 0) {
		cmd[cmd_append_pos] = NULL;
		for(i = 0, n = strlen(buf); cmd[i] && n < sizeof(buf); i++)
			n += snprintf(&buf[n], sizeof(buf) - n, " %s", cmd[i]);

		xsettitle(win, buf);
		#if ICON_PATCH
		XChangeProperty(dpy, win, wmatom[WMIcon], XA_CARDINAL, 32,
				PropModeReplace, (unsigned char *) icon, ICON_WIDTH * ICON_HEIGHT + 2);
		#endif // ICON_PATCH
		XRaiseWindow(dpy, win);

		return;
	}

	if (c < 0 || c >= nclients)
		return;

	resize(c, ww, wh - bh);
	XRaiseWindow(dpy, clients[c]->win);
	XSetInputFocus(dpy, clients[c]->win, RevertToParent, CurrentTime);
	sendxembed(c, XEMBED_FOCUS_IN, XEMBED_FOCUS_CURRENT, 0, 0);
	sendxembed(c, XEMBED_WINDOW_ACTIVATE, 0, 0, 0);
	xsettitle(win, clients[c]->name);

	if (sel != c) {
		lastsel = sel;
		sel = c;
	}
	#if ICON_PATCH
	xseticon();
	#endif // ICON_PATCH

	if (clients[c]->urgent && (wmh = XGetWMHints(dpy, clients[c]->win))) {
		wmh->flags &= ~XUrgencyHint;
		XSetWMHints(dpy, clients[c]->win, wmh);
		clients[c]->urgent = False;
		XFree(wmh);

		/*
		 * gnome-shell will not stop notifying us about urgency,
		 * if we clear only the client hint and don't clear the
		 * hint from the main container window
		 */
		if ((win_wmh = XGetWMHints(dpy, win))) {
			win_wmh->flags &= ~XUrgencyHint;
			XSetWMHints(dpy, win, win_wmh);
			XFree(win_wmh);
		}
	}

	drawbar();
	XSync(dpy, False);
}

void
focusin(const XEvent *e)
{
	const XFocusChangeEvent *ev = &e->xfocus;
	int dummy;
	Window focused;

	if (ev->mode != NotifyUngrab) {
		XGetInputFocus(dpy, &focused, &dummy);
		if (focused == win)
			focus(sel);
	}
}

void
focusonce(const Arg *arg)
{
	nextfocus = True;
}

void
focusurgent(const Arg *arg)
{
	int c;

	if (sel < 0)
		return;

	for (c = (sel + 1) % nclients; c != sel; c = (c + 1) % nclients) {
		if (clients[c]->urgent) {
			focus(c);
			return;
		}
	}
}

void
fullscreen(const Arg *arg)
{
	XEvent e;

	e.type = ClientMessage;
	e.xclient.window = win;
	e.xclient.message_type = wmatom[WMState];
	e.xclient.format = 32;
	e.xclient.data.l[0] = 2;
	e.xclient.data.l[1] = wmatom[WMFullscreen];
	e.xclient.data.l[2] = 0;
	XSendEvent(dpy, root, False, SubstructureNotifyMask, &e);
}

char *
getatom(int a)
{
	static char buf[BUFSIZ];
	Atom adummy;
	int idummy;
	unsigned long ldummy;
	unsigned char *p = NULL;

	XGetWindowProperty(dpy, win, wmatom[a], 0L, BUFSIZ, False, XA_STRING,
	                   &adummy, &idummy, &ldummy, &ldummy, &p);
	if (p)
		strncpy(buf, (char *)p, LENGTH(buf)-1);
	else
		buf[0] = '\0';
	XFree(p);

	return buf;
}

int
getclient(Window w)
{
	int i;

	for (i = 0; i < nclients; i++) {
		if (clients[i]->win == w)
			return i;
	}

	return -1;
}

XftColor
getcolor(const char *colstr)
{
	XftColor color;

	#if ALPHA_PATCH
	if (!XftColorAllocName(dpy, visual, cmap, colstr, &color))
	#else
	if (!XftColorAllocName(dpy, DefaultVisual(dpy, screen), DefaultColormap(dpy, screen), colstr, &color))
		#endif // ALPHA_PATCH
		die("%s: cannot allocate color '%s'\n", argv0, colstr);

	return color;
}

int
getfirsttab(void)
{
	int cc, ret;

	if (sel < 0)
		return 0;

	cc = ww / tabwidth;
	if (nclients > cc)
		cc = (ww - TEXTW(before) - TEXTW(after)) / tabwidth;

	ret = sel - cc / 2 + (cc + 1) % 2;
	return ret < 0 ? 0 :
	       ret + cc > nclients ? MAX(0, nclients - cc) :
	       ret;
}

Bool
gettextprop(Window w, Atom atom, char *text, unsigned int size)
{
	char **list = NULL;
	int n;
	XTextProperty name;

	if (!text || size == 0)
		return False;

	text[0] = '\0';
	XGetTextProperty(dpy, w, &name, atom);
	if (!name.nitems)
		return False;

	if (name.encoding == XA_STRING) {
		strncpy(text, (char *)name.value, size - 1);
	} else if (XmbTextPropertyToTextList(dpy, &name, &list, &n) >= Success
	           && n > 0 && *list) {
		strncpy(text, *list, size - 1);
		XFreeStringList(list);
	}
	text[size - 1] = '\0';
	XFree(name.value);

	return True;
}

void
initfont(const char *fontstr)
{
	if (!(dc.font.xfont = XftFontOpenName(dpy, screen, fontstr))
	    && !(dc.font.xfont = XftFontOpenName(dpy, screen, "fixed")))
		die("error, cannot load font: '%s'\n", fontstr);

	dc.font.ascent = dc.font.xfont->ascent;
	dc.font.descent = dc.font.xfont->descent;
	dc.font.height = dc.font.ascent + dc.font.descent;
}

Bool
isprotodel(int c)
{
	int i, n;
	Atom *protocols;
	Bool ret = False;

	if (XGetWMProtocols(dpy, clients[c]->win, &protocols, &n)) {
		for (i = 0; !ret && i < n; i++) {
			if (protocols[i] == wmatom[WMDelete])
				ret = True;
		}
		XFree(protocols);
	}

	return ret;
}

void
keypress(const XEvent *e)
{
	const XKeyEvent *ev = &e->xkey;
	unsigned int i;
	#if !KEYCODE_PATCH
	KeySym keysym;

	keysym = XkbKeycodeToKeysym(dpy, (KeyCode)ev->keycode, 0, 0);
	#endif // KEYCODE_PATCH
	for (i = 0; i < LENGTH(keys); i++) {
		if (
			#if KEYCODE_PATCH
			ev->keycode == keys[i].keycode &&
			#else
			keysym == keys[i].keysym &&
			#endif // KEYCODE_PATCH
		    CLEANMASK(keys[i].mod) == CLEANMASK(ev->state) &&
		    keys[i].func)
			keys[i].func(&(keys[i].arg));
	}
}

void
killclient(const Arg *arg)
{
	XEvent ev;

	if (sel < 0)
		return;

	if (isprotodel(sel) && !clients[sel]->closed) {
		ev.type = ClientMessage;
		ev.xclient.window = clients[sel]->win;
		ev.xclient.message_type = wmatom[WMProtocols];
		ev.xclient.format = 32;
		ev.xclient.data.l[0] = wmatom[WMDelete];
		ev.xclient.data.l[1] = CurrentTime;
		XSendEvent(dpy, clients[sel]->win, False, NoEventMask, &ev);
		clients[sel]->closed = True;
	} else {
		XKillClient(dpy, clients[sel]->win);
	}
}

void
manage(Window w)
{
	updatenumlockmask();
	{
		int i, j, nextpos;
		unsigned int modifiers[] = { 0, LockMask, numlockmask,
		                             numlockmask | LockMask };
		#if !KEYCODE_PATCH
		KeyCode code;
		#endif // KEYCODE_PATCH
		Client *c;
		XEvent e;

		XWithdrawWindow(dpy, w, 0);
		XReparentWindow(dpy, w, win, 0, bh);
		XSelectInput(dpy, w, PropertyChangeMask |
		             StructureNotifyMask | EnterWindowMask);
		XSync(dpy, False);

		for (i = 0; i < LENGTH(keys); i++) {
			#if KEYCODE_PATCH
			for (j = 0; j < LENGTH(modifiers); ++j) {
				XGrabKey(dpy, keys[i].keycode,
				         keys[i].mod | modifiers[j], w,
				         True, GrabModeAsync, GrabModeAsync);
 			}
			#else
			if ((code = XKeysymToKeycode(dpy, keys[i].keysym))) {
				for (j = 0; j < LENGTH(modifiers); j++) {
					XGrabKey(dpy, code, keys[i].mod |
					         modifiers[j], w, True,
					         GrabModeAsync, GrabModeAsync);
				}
			}
			#endif // KEYCODE_PATCH
		}

		#if KEYRELEASE_PATCH
		for (i = 0; i < LENGTH(keyreleases); i++) {
			if ((code = XKeysymToKeycode(dpy, keyreleases[i].keysym))) {
				for (j = 0; j < LENGTH(modifiers); j++) {
					XGrabKey(dpy, code, keyreleases[i].mod |
					         modifiers[j], w, True,
					         GrabModeAsync, GrabModeAsync);
				}
			}
		}
		#endif // KEYRELEASE_PATCH

		c = ecalloc(1, sizeof *c);
		c->win = w;

		nclients++;
		clients = erealloc(clients, sizeof(Client *) * nclients);

		if(npisrelative) {
			nextpos = sel + newposition;
		} else {
			if (newposition < 0)
				nextpos = nclients - newposition;
			else
				nextpos = newposition;
		}
		if (nextpos >= nclients)
			nextpos = nclients - 1;
		if (nextpos < 0)
			nextpos = 0;

		if (nclients > 1 && nextpos < nclients - 1)
			memmove(&clients[nextpos + 1], &clients[nextpos],
			        sizeof(Client *) * (nclients - nextpos - 1));

		clients[nextpos] = c;
		updatetitle(nextpos);

		XLowerWindow(dpy, w);
		XMapWindow(dpy, w);

		e.xclient.window = w;
		e.xclient.type = ClientMessage;
		e.xclient.message_type = wmatom[XEmbed];
		e.xclient.format = 32;
		e.xclient.data.l[0] = CurrentTime;
		e.xclient.data.l[1] = XEMBED_EMBEDDED_NOTIFY;
		e.xclient.data.l[2] = 0;
		e.xclient.data.l[3] = win;
		e.xclient.data.l[4] = 0;
		XSendEvent(dpy, root, False, NoEventMask, &e);

		XSync(dpy, False);

		/* Adjust sel before focus does set it to lastsel. */
		if (sel >= nextpos)
			sel++;
		focus(nextfocus ? nextpos :
		      sel < 0 ? 0 :
		      sel);
		nextfocus = foreground;
	}
}

void
maprequest(const XEvent *e)
{
	const XMapRequestEvent *ev = &e->xmaprequest;

	if (getclient(ev->window) < 0)
		manage(ev->window);
}

void
move(const Arg *arg)
{
	if (arg->i >= 0 && arg->i < nclients)
		focus(arg->i);
}

void
movetab(const Arg *arg)
{
	int c;
	Client *new;

	if (sel < 0)
		return;

	c = (sel + arg->i) % nclients;
	if (c < 0)
		c += nclients;

	if (c == sel)
		return;

	new = clients[sel];
	if (sel < c)
		memmove(&clients[sel], &clients[sel+1],
		        sizeof(Client *) * (c - sel));
	else
		memmove(&clients[c+1], &clients[c],
		        sizeof(Client *) * (sel - c));
	clients[c] = new;
	sel = c;

	drawbar();
}

void
propertynotify(const XEvent *e)
{
	const XPropertyEvent *ev = &e->xproperty;
	XWMHints *wmh;
	int c;
	char* selection = NULL;
	Arg arg;

	if (ev->state == PropertyNewValue && ev->atom == wmatom[WMSelectTab]) {
		selection = getatom(WMSelectTab);
		if (!strncmp(selection, "0x", 2)) {
			arg.i = getclient(strtoul(selection, NULL, 0));
			move(&arg);
		} else {
			cmd[cmd_append_pos] = selection;
			arg.v = cmd;
			spawn(&arg);
		}
	} else if (ev->state == PropertyNewValue && ev->atom == XA_WM_HINTS &&
	           (c = getclient(ev->window)) > -1 &&
	           (wmh = XGetWMHints(dpy, clients[c]->win))) {
		if (wmh->flags & XUrgencyHint) {
			XFree(wmh);
			wmh = XGetWMHints(dpy, win);
			if (c != sel) {
				if (urgentswitch && wmh &&
				    !(wmh->flags & XUrgencyHint)) {
					/* only switch, if tabbed was focused
					 * since last urgency hint if WMHints
					 * could not be received,
					 * default to no switch */
					focus(c);
				} else {
					/* if no switch should be performed,
					 * mark tab as urgent */
					clients[c]->urgent = True;
					drawbar();
				}
			}
			if (wmh && !(wmh->flags & XUrgencyHint)) {
				/* update tabbed urgency hint
				 * if not set already */
				wmh->flags |= XUrgencyHint;
				XSetWMHints(dpy, win, wmh);
			}
		}
		XFree(wmh);
		#if ICON_PATCH
		if (c == sel)
			xseticon();
		#endif // ICON_PATCH
	} else if (ev->state != PropertyDelete && ev->atom == XA_WM_NAME &&
	           (c = getclient(ev->window)) > -1) {
		updatetitle(c);
	#if ICON_PATCH
	} else if (ev->atom == wmatom[WMIcon] && (c = getclient(ev->window)) > -1 && c == sel) {
		xseticon();
	#endif // ICON_PATCH
	}
}

void
resize(int c, int w, int h)
{
	XConfigureEvent ce;
	XWindowChanges wc;

	ce.x = 0;
	#if BOTTOM_TABS_PATCH
	ce.y = wc.y = 0;
	#else
	ce.y = wc.y = bh;
	#endif // BOTTOM_TABS_PATCH
	ce.width = wc.width = w;
	ce.height = wc.height = h;
	ce.type = ConfigureNotify;
	ce.display = dpy;
	ce.event = clients[c]->win;
	ce.window = clients[c]->win;
	ce.above = None;
	ce.override_redirect = False;
	ce.border_width = 0;

	XConfigureWindow(dpy, clients[c]->win, CWY | CWWidth | CWHeight, &wc);
	XSendEvent(dpy, clients[c]->win, False, StructureNotifyMask,
	           (XEvent *)&ce);
}

void
rotate(const Arg *arg)
{
	int nsel = -1;

	if (sel < 0)
		return;

	if (arg->i == 0) {
		if (lastsel > -1)
			focus(lastsel);
	} else if (sel > -1) {
		/* Rotating in an arg->i step around the clients. */
		nsel = sel + arg->i;
		while (nsel >= nclients)
			nsel -= nclients;
		while (nsel < 0)
			nsel += nclients;
		focus(nsel);
	}
}

void
run(void)
{
	XEvent ev;

	/* main event loop */
	XSync(dpy, False);
	drawbar();
	if (doinitspawn == True)
		spawn(NULL);

	while (running) {
		XNextEvent(dpy, &ev);
		if (handler[ev.type])
			(handler[ev.type])(&ev); /* call handler */
	}
}

void
sendxembed(int c, long msg, long detail, long d1, long d2)
{
	XEvent e = { 0 };

	e.xclient.window = clients[c]->win;
	e.xclient.type = ClientMessage;
	e.xclient.message_type = wmatom[XEmbed];
	e.xclient.format = 32;
	e.xclient.data.l[0] = CurrentTime;
	e.xclient.data.l[1] = msg;
	e.xclient.data.l[2] = detail;
	e.xclient.data.l[3] = d1;
	e.xclient.data.l[4] = d2;
	XSendEvent(dpy, clients[c]->win, False, NoEventMask, &e);
}

void
setcmd(int argc, char *argv[], int replace)
{
	int i;

	cmd = ecalloc(argc + 3, sizeof(*cmd));
	if (argc == 0)
		return;
	for (i = 0; i < argc; i++)
		cmd[i] = argv[i];
	cmd[replace > 0 ? replace : argc] = winid;
	cmd_append_pos = argc + !replace;
	cmd[cmd_append_pos] = cmd[cmd_append_pos + 1] = NULL;
}

void
setup(void)
{
	int bitm, tx, ty, tw, th, dh, dw, isfixed;
	XWMHints *wmh;
	XClassHint class_hint;
	XSizeHints *size_hint;
	struct sigaction sa;

	/* do not transform children into zombies when they terminate */
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_NOCLDSTOP | SA_NOCLDWAIT | SA_RESTART;
	sa.sa_handler = SIG_IGN;
	sigaction(SIGCHLD, &sa, NULL);

	/* clean up any zombies that might have been inherited */
	while (waitpid(-1, NULL, WNOHANG) > 0);

	/* init screen */
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
	initfont(font);
	#if BAR_HEIGHT_PATCH
	dc.h = (barheight ? barheight : dc.font.height + 2);
	#else
	dc.h = dc.font.height + 2;
	#endif // BAR_HEIGHT_PATCH
	#if AUTOHIDE_PATCH || HIDETABS_PATCH
	vbh = dc.h;
	#else
	bh = dc.h;
	#endif // AUTOHIDE_PATCH | HIDETABS_PATCH

	/* init atoms */
	wmatom[WMDelete] = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	wmatom[WMFullscreen] = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN",
	                                   False);
	wmatom[WMName] = XInternAtom(dpy, "_NET_WM_NAME", False);
	#if CLASS_PATCH
	wmatom[WMName] = XInternAtom(dpy, "_NET_WM_NAME", False);
	#endif // CLASS_PATCH
	wmatom[WMProtocols] = XInternAtom(dpy, "WM_PROTOCOLS", False);
	wmatom[WMSelectTab] = XInternAtom(dpy, "_TABBED_SELECT_TAB", False);
	wmatom[WMState] = XInternAtom(dpy, "_NET_WM_STATE", False);
	wmatom[XEmbed] = XInternAtom(dpy, "_XEMBED", False);
	#if ICON_PATCH
	wmatom[WMIcon] = XInternAtom(dpy, "_NET_WM_ICON", False);
	#endif // ICON_PATCH

	/* init appearance */
	wx = 0;
	wy = 0;
	ww = 800;
	wh = 600;
	isfixed = 0;

	if (geometry) {
		tx = ty = tw = th = 0;
		bitm = XParseGeometry(geometry, &tx, &ty, (unsigned *)&tw,
		                      (unsigned *)&th);
		if (bitm & XValue)
			wx = tx;
		if (bitm & YValue)
			wy = ty;
		if (bitm & WidthValue)
			ww = tw;
		if (bitm & HeightValue)
			wh = th;
		if (bitm & XNegative && wx == 0)
			wx = -1;
		if (bitm & YNegative && wy == 0)
			wy = -1;
		if (bitm & (HeightValue | WidthValue))
			isfixed = 1;

		dw = DisplayWidth(dpy, screen);
		dh = DisplayHeight(dpy, screen);
		if (wx < 0)
			wx = dw + wx - ww - 1;
		if (wy < 0)
			wy = dh + wy - wh - 1;
	}

	#if ALPHA_PATCH
	XVisualInfo *vis;
	XRenderPictFormat *fmt;
	int nvi;
	int i;

	XVisualInfo tpl = {
		.screen = screen,
		.depth = 32,
		.class = TrueColor
	};

	vis = XGetVisualInfo(dpy, VisualScreenMask | VisualDepthMask | VisualClassMask, &tpl, &nvi);
	for(i = 0; i < nvi; i ++) {
		fmt = XRenderFindVisualFormat(dpy, vis[i].visual);
		if (fmt->type == PictTypeDirect && fmt->direct.alphaMask) {
			visual = vis[i].visual;
			break;
		}
	}

	XFree(vis);

	if (! visual) {
		fprintf(stderr, "Couldn't find ARGB visual.\n");
		exit(1);
	}

	cmap = XCreateColormap( dpy, root, visual, None);
	#endif // ALPHA_PATCH

	dc.norm[ColBG] = getcolor(normbgcolor);
	dc.norm[ColFG] = getcolor(normfgcolor);
	dc.sel[ColBG] = getcolor(selbgcolor);
	dc.sel[ColFG] = getcolor(selfgcolor);
	dc.urg[ColBG] = getcolor(urgbgcolor);
	dc.urg[ColFG] = getcolor(urgfgcolor);
	#if ALPHA_PATCH
	XSetWindowAttributes attrs;
	attrs.background_pixel = dc.norm[ColBG].pixel;
	attrs.border_pixel = dc.norm[ColFG].pixel;
	attrs.bit_gravity = NorthWestGravity;
	attrs.event_mask = FocusChangeMask | KeyPressMask
		| ExposureMask | VisibilityChangeMask | StructureNotifyMask
		| ButtonMotionMask | ButtonPressMask | ButtonReleaseMask;
	attrs.background_pixmap = None;
	attrs.colormap = cmap;

	win = XCreateWindow(dpy, root, wx, wy, ww, wh, 0, 32, InputOutput,
		visual, CWBackPixmap | CWBorderPixel | CWBitGravity
		| CWEventMask | CWColormap, &attrs);

	dc.drawable = XCreatePixmap(dpy, win, ww, wh, 32);
	dc.gc = XCreateGC(dpy, dc.drawable, 0, 0);
	#else
	dc.drawable = XCreatePixmap(dpy, root, ww, wh,
	                            DefaultDepth(dpy, screen));
	dc.gc = XCreateGC(dpy, root, 0, 0);

	win = XCreateSimpleWindow(dpy, root, wx, wy, ww, wh, 0,
	                          dc.norm[ColFG].pixel, dc.norm[ColBG].pixel);
	#endif // ALPHA_PATCH
	XMapRaised(dpy, win);
	XSelectInput(dpy, win, SubstructureNotifyMask | FocusChangeMask |
	             ButtonPressMask | ExposureMask | KeyPressMask |
	             #if KEYRELEASE_PATCH
	             KeyReleaseMask |
	             #endif // KEYRELEASE_PATCH
	             PropertyChangeMask | StructureNotifyMask |
	             #if DRAG_PATCH
	             ButtonMotionMask |
	             #endif // DRAG_PATCH
	             SubstructureRedirectMask);
	xerrorxlib = XSetErrorHandler(xerror);

	class_hint.res_name = wmname;
	#if CLASS_PATCH
	class_hint.res_class = wmclass;
	#else
	class_hint.res_class = "tabbed";
	#endif // CLASS_PATCH
	XSetClassHint(dpy, win, &class_hint);

	size_hint = XAllocSizeHints();
	if (!isfixed) {
		size_hint->flags = PSize | PMinSize;
		size_hint->height = wh;
		size_hint->width = ww;
		size_hint->min_height = bh + 1;
	} else {
		size_hint->flags = PMaxSize | PMinSize;
		size_hint->min_width = size_hint->max_width = ww;
		size_hint->min_height = size_hint->max_height = wh;
	}
	wmh = XAllocWMHints();
	XSetWMProperties(dpy, win, NULL, NULL, NULL, 0, size_hint, wmh, NULL);
	XFree(size_hint);
	XFree(wmh);

	XSetWMProtocols(dpy, win, &wmatom[WMDelete], 1);

	snprintf(winid, sizeof(winid), "%lu", win);
	setenv("XEMBED", winid, 1);

	#if ICON_PATCH
	/* change icon from RGBA to ARGB */
	icon[0] = ICON_WIDTH;
	icon[1] =  ICON_HEIGHT;
	for (int i = 0; i < ICON_WIDTH * ICON_HEIGHT; ++i) {
		icon[i + 2] =
			ICON_PIXEL_DATA[i * 4 + 3] << 24 |
			ICON_PIXEL_DATA[i * 4 + 0] <<  0 |
			ICON_PIXEL_DATA[i * 4 + 1] <<  8 |
			ICON_PIXEL_DATA[i * 4 + 2] << 16 ;
	}
	#endif // ICON_PATCH

	nextfocus = foreground;
	focus(-1);
}

void
spawn(const Arg *arg)
{
	struct sigaction sa;

	if (fork() == 0) {
		if(dpy)
			close(ConnectionNumber(dpy));

		setsid();

		sigemptyset(&sa.sa_mask);
		sa.sa_flags = 0;
		sa.sa_handler = SIG_DFL;
		sigaction(SIGCHLD, &sa, NULL);

		if (arg && arg->v) {
			execvp(((char **)arg->v)[0], (char **)arg->v);
			fprintf(stderr, "%s: execvp %s", argv0,
			        ((char **)arg->v)[0]);
		} else {
			cmd[cmd_append_pos] = NULL;
			execvp(cmd[0], cmd);
			fprintf(stderr, "%s: execvp %s", argv0, cmd[0]);
		}
		perror(" failed");
		exit(0);
	}
}

int
textnw(const char *text, unsigned int len)
{
	XGlyphInfo ext;
	XftTextExtentsUtf8(dpy, dc.font.xfont, (XftChar8 *) text, len, &ext);
	return ext.xOff;
}

void
toggle(const Arg *arg)
{
    *(Bool*) arg->v = !*(Bool*) arg->v;
}

void
unmanage(int c)
{
	if (c < 0 || c >= nclients) {
		drawbar();
		XSync(dpy, False);
		return;
	}

	if (!nclients)
		return;

	if (c == 0) {
		/* First client. */
		nclients--;
		free(clients[0]);
		memmove(&clients[0], &clients[1], sizeof(Client *) * nclients);
	} else if (c == nclients - 1) {
		/* Last client. */
		nclients--;
		free(clients[c]);
		clients = erealloc(clients, sizeof(Client *) * nclients);
	} else {
		/* Somewhere inbetween. */
		free(clients[c]);
		memmove(&clients[c], &clients[c+1],
		        sizeof(Client *) * (nclients - (c + 1)));
		nclients--;
	}

	if (nclients <= 0) {
		lastsel = sel = -1;

		if (closelastclient)
			running = False;
		else if (fillagain && running)
			spawn(NULL);
	} else {
		if (lastsel >= nclients)
			lastsel = nclients - 1;
		else if (lastsel > c)
			lastsel--;

		if (c == sel && lastsel >= 0) {
			focus(lastsel);
		} else {
			if (sel > c)
				sel--;
			if (sel >= nclients)
				sel = nclients - 1;

			focus(sel);
		}
	}

	drawbar();
	XSync(dpy, False);
}

void
unmapnotify(const XEvent *e)
{
	const XUnmapEvent *ev = &e->xunmap;
	int c;

	if ((c = getclient(ev->window)) > -1)
		unmanage(c);
}

void
updatenumlockmask(void)
{
	unsigned int i, j;
	XModifierKeymap *modmap;

	numlockmask = 0;
	modmap = XGetModifierMapping(dpy);
	for (i = 0; i < 8; i++) {
		for (j = 0; j < modmap->max_keypermod; j++) {
			if (modmap->modifiermap[i * modmap->max_keypermod + j]
			    == XKeysymToKeycode(dpy, XK_Num_Lock))
				numlockmask = (1 << i);
		}
	}
	XFreeModifiermap(modmap);
}

void
updatetitle(int c)
{
	if (!gettextprop(clients[c]->win, wmatom[WMName], clients[c]->name,
	    sizeof(clients[c]->name)))
		gettextprop(clients[c]->win, XA_WM_NAME, clients[c]->name,
		            sizeof(clients[c]->name));
	#if BASENAME_PATCH
	if (basenametitles)
		clients[c]->basename = getbasename(clients[c]->name);
	#endif // BASENAME_PATCH
	if (sel == c)
		xsettitle(win, clients[c]->name);
	drawbar();
}

/* There's no way to check accesses to destroyed windows, thus those cases are
 * ignored (especially on UnmapNotify's).  Other types of errors call Xlibs
 * default error handler, which may call exit.  */
int
xerror(Display *dpy, XErrorEvent *ee)
{
	if (ee->error_code == BadWindow
	    || (ee->request_code == X_SetInputFocus &&
	        ee->error_code == BadMatch)
	    || (ee->request_code == X_PolyText8 &&
	        ee->error_code == BadDrawable)
	    || (ee->request_code == X_PolyFillRectangle &&
	        ee->error_code == BadDrawable)
	    || (ee->request_code == X_PolySegment &&
	        ee->error_code == BadDrawable)
	    || (ee->request_code == X_ConfigureWindow &&
	        ee->error_code == BadMatch)
	    || (ee->request_code == X_GrabButton &&
	        ee->error_code == BadAccess)
	    || (ee->request_code == X_GrabKey &&
	        ee->error_code == BadAccess)
	    || (ee->request_code == X_CopyArea &&
	        ee->error_code == BadDrawable))
		return 0;

	fprintf(stderr, "%s: fatal error: request code=%d, error code=%d\n",
	        argv0, ee->request_code, ee->error_code);
	return xerrorxlib(dpy, ee); /* may call exit */
}

void
xsettitle(Window w, const char *str)
{
	XTextProperty xtp;

	if (XmbTextListToTextProperty(dpy, (char **)&str, 1,
	    XUTF8StringStyle, &xtp) == Success) {
		XSetTextProperty(dpy, w, &xtp, wmatom[WMName]);
		XSetTextProperty(dpy, w, &xtp, XA_WM_NAME);
		XFree(xtp.value);
	}
}

void
usage(void)
{
	die("usage: %s [-"
		#if BASENAME_PATCH
		"b"
		#endif // BASENAME_PATCH
		"dfksv] [-g geometry] [-n name] [-p [s+/-]pos]\n"
	    "       [-r narg] [-o color] [-O color] [-t color] [-T color]\n"
	    "       [-u color] [-U color] command...\n", argv0);
}

int
main(int argc, char *argv[])
{
	Bool detach = False;
	int replace = 0;
	char *pstr;

	ARGBEGIN {
	case 'c':
		closelastclient = True;
		fillagain = False;
		break;
	case 'd':
		detach = True;
		break;
	case 'f':
		fillagain = True;
		break;
	case 'g':
		geometry = EARGF(usage());
		break;
	case 'k':
		killclientsfirst = True;
		break;
	case 'n':
		wmname = EARGF(usage());
		#if CLASS_PATCH
		wmclass = wmname;
		#endif // CLASS_PATCH
		break;
	case 'O':
		normfgcolor = EARGF(usage());
		break;
	case 'o':
		normbgcolor = EARGF(usage());
		break;
	case 'p':
		pstr = EARGF(usage());
		if (pstr[0] == 's') {
			npisrelative = True;
			newposition = atoi(&pstr[1]);
		} else {
			newposition = atoi(pstr);
		}
		break;
	case 'r':
		replace = atoi(EARGF(usage()));
		break;
	case 's':
		doinitspawn = False;
		break;
	case 'T':
		selfgcolor = EARGF(usage());
		break;
	case 't':
		selbgcolor = EARGF(usage());
		break;
	case 'U':
		urgfgcolor = EARGF(usage());
		break;
	case 'u':
		urgbgcolor = EARGF(usage());
		break;
	#if BASENAME_PATCH
	case 'b':
		basenametitles = True;
		break;
	#endif // BASENAME_PATCH
	case 'v':
		die("tabbed-"VERSION", © 2009-2016 tabbed engineers, "
		    "see LICENSE for details.\n");
		break;
	default:
		usage();
		break;
	} ARGEND;

	if (argc < 1) {
		doinitspawn = False;
		fillagain = False;
	}

	setcmd(argc, argv, replace);

	if (!setlocale(LC_CTYPE, "") || !XSupportsLocale())
		fprintf(stderr, "%s: no locale support\n", argv0);
	if (!(dpy = XOpenDisplay(NULL)))
		die("%s: cannot open display\n", argv0);

	#if XRESOURCES_PATCH
	config_init();
	#if XRESOURCES_RELOAD_PATCH
	signal(SIGUSR1, xrdb_reload);
	#endif // XRESOURCES_RELOAD_PATCH
	#endif // XRESOURCES_PATCH
	setup();
	printf("0x%lx\n", win);
	fflush(NULL);

	if (detach) {
		if (fork() == 0) {
			fclose(stdout);
		} else {
			if (dpy)
				close(ConnectionNumber(dpy));
			return EXIT_SUCCESS;
		}
	}

	run();
	cleanup();
	XCloseDisplay(dpy);

	return EXIT_SUCCESS;
}
