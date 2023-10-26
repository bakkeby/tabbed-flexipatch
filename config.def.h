/* See LICENSE file for copyright and license details. */

/* appearance */
static char *font         = "monospace:size=9";
static char *normbgcolor  = "#222222";
static char *normfgcolor  = "#cccccc";
static char *selbgcolor   = "#555555";
static char *selfgcolor   = "#ffffff";
static char *urgbgcolor   = "#111111";
static char *urgfgcolor   = "#cc0000";
static char before[]      = "<";
static char after[]       = ">";
static char titletrim[]   = "...";
static int  tabwidth      = 200;
static int  foreground    = 1;
static int  urgentswitch  = 0;
#if SEPARATOR_PATCH
static int  separator     = 4;
#endif // SEPARATOR_PATCH

#if BAR_HEIGHT_PATCH
static int barheight = 0;  /* 0 means derive by font (default), otherwise absolute height */
#endif // BAR_HEIGHT_PATCH

/*
 * Where to place a new tab when it is opened. When npisrelative is 1,
 * then the current position is changed + newposition. If npisrelative
 * is 0, then newposition is an absolute position.
 */
static int newposition  = 0;
static int npisrelative = 0;

#define SETPROP(p) { \
        .v = (char *[]){ "/bin/sh", "-c", \
                "prop=\"`xwininfo -children -id $1 | grep '^     0x' |" \
                "sed -e's@^ *\\(0x[0-9a-f]*\\) \"\\([^\"]*\\)\".*@\\1 \\2@' |" \
                "xargs -0 printf %b | dmenu -l 10 -w $1`\" &&" \
                "xprop -id $1 -f $0 8s -set $0 \"$prop\"", \
                p, winid, NULL \
        } \
}

#if XRESOURCES_PATCH
/*
 * Xresources preferences to load at startup
 */
ResourcePref resources[] = {
	{ "font",         STRING,  &font },
	{ "color0",       STRING,  &normbgcolor },
	{ "color4",       STRING,  &normfgcolor },
	{ "color4",       STRING,  &selbgcolor },
	{ "color7",       STRING,  &selfgcolor },
	{ "color2",       STRING,  &urgbgcolor },
	{ "color3",       STRING,  &urgfgcolor },
	{ "before",       STRING,  &before },
	{ "after",        STRING,  &after },
	{ "titletrim",    STRING,  &titletrim },
	{ "tabwidth",     INTEGER, &tabwidth },
	{ "foreground",   INTEGER, &foreground },
	{ "urgentswitch", INTEGER, &urgentswitch },
	{ "newposition",  INTEGER, &newposition },
	{ "npisrelative", INTEGER, &npisrelative },
	#if SEPARATOR_PATCH
	{ "separator",    INTEGER, &separator },
	#endif // SEPARATOR_PATCH
	#if BAR_HEIGHT_PATCH
	{ "barheight",    INTEGER, &barheight },
	#endif // BAR_HEIGHT_PATCH
};
#endif // XRESOURCES_PATCH

#define MODKEY ControlMask
#if KEYCODE_PATCH
static const Key keys[] = {
	/* modifier             key           function     argument */
	{ MODKEY|ShiftMask,     36,           focusonce,   { 0 } },
	{ MODKEY|ShiftMask,     36,           spawn,       { 0 } },
	{ MODKEY|ShiftMask,     46,           rotate,      { .i = +1 } },
	{ MODKEY|ShiftMask,     43,           rotate,      { .i = -1 } },
	{ MODKEY|ShiftMask,     44,           movetab,     { .i = -1 } },
	{ MODKEY|ShiftMask,     45,           movetab,     { .i = +1 } },
	{ MODKEY,               23,           rotate,      { .i = 0 } },
	{ MODKEY,               49,           spawn,       SETPROP("_TABBED_SELECT_TAB") },
	{ MODKEY,               10,           move,        { .i = 0 } },
	{ MODKEY,               11,           move,        { .i = 1 } },
	{ MODKEY,               12,           move,        { .i = 2 } },
	{ MODKEY,               13,           move,        { .i = 3 } },
	{ MODKEY,               14,           move,        { .i = 4 } },
	{ MODKEY,               15,           move,        { .i = 5 } },
	{ MODKEY,               16,           move,        { .i = 6 } },
	{ MODKEY,               17,           move,        { .i = 7 } },
	{ MODKEY,               18,           move,        { .i = 8 } },
	{ MODKEY,               19,           move,        { .i = 9 } },
	{ MODKEY,               24,           killclient,  { 0 } },
	{ MODKEY,               30,           focusurgent, { .v = NULL } },
	{ MODKEY|ShiftMask,     30,           toggle,      { .v = (void*) &urgentswitch } },
	{ 0,                    95,           fullscreen,  { 0 } },
	#if HIDETABS_PATCH
	{ MODKEY,               50,           showbar,     { .i = 1 } },
	{ ShiftMask,            37,           showbar,     { .i = 1 } },
	#endif // HIDETABS_PATCH
};
#else
static const Key keys[] = {
	/* modifier             key           function     argument */
	{ MODKEY|ShiftMask,     XK_Return,    focusonce,   { 0 } },
	{ MODKEY|ShiftMask,     XK_Return,    spawn,       { 0 } },

	{ MODKEY|ShiftMask,     XK_l,         rotate,      { .i = +1 } },
	{ MODKEY|ShiftMask,     XK_h,         rotate,      { .i = -1 } },
	{ MODKEY|ShiftMask,     XK_j,         movetab,     { .i = -1 } },
	{ MODKEY|ShiftMask,     XK_k,         movetab,     { .i = +1 } },
	{ MODKEY,               XK_Tab,       rotate,      { .i = 0 } },

	{ MODKEY,               XK_grave,     spawn,       SETPROP("_TABBED_SELECT_TAB") },
	{ MODKEY,               XK_1,         move,        { .i = 0 } },
	{ MODKEY,               XK_2,         move,        { .i = 1 } },
	{ MODKEY,               XK_3,         move,        { .i = 2 } },
	{ MODKEY,               XK_4,         move,        { .i = 3 } },
	{ MODKEY,               XK_5,         move,        { .i = 4 } },
	{ MODKEY,               XK_6,         move,        { .i = 5 } },
	{ MODKEY,               XK_7,         move,        { .i = 6 } },
	{ MODKEY,               XK_8,         move,        { .i = 7 } },
	{ MODKEY,               XK_9,         move,        { .i = 8 } },
	{ MODKEY,               XK_0,         move,        { .i = 9 } },

	{ MODKEY,               XK_q,         killclient,  { 0 } },

	{ MODKEY,               XK_u,         focusurgent, { 0 } },
	{ MODKEY|ShiftMask,     XK_u,         toggle,      { .v = (void*) &urgentswitch } },

	{ 0,                    XK_F11,       fullscreen,  { 0 } },
	#if HIDETABS_PATCH
	{ MODKEY,               XK_Shift_L,   showbar,     { .i = 1 } },
	{ ShiftMask,            XK_Control_L, showbar,     { .i = 1 } },
	#endif // HIDETABS_PATCH
};
#endif // KEYCODE_PATCH

#if KEYRELEASE_PATCH
static const Key keyreleases[] = {
	/* modifier             key             function     argument */
	#if HIDETABS_PATCH
	{ MODKEY|ShiftMask,     XK_Shift_L,     showbar,     { .i = 0 } },
	{ MODKEY|ShiftMask,     XK_Control_L,   showbar,     { .i = 0 } },
	#else
	{ 0,                    XK_Shift_L,     NULL,        { 0 } },
	#endif // HIDETABS_PATCH

};
#endif // KEYRELEASE_PATCH