/*
 * This file contains patch control flags.
 *
 * In principle you should be able to mix and match any patches
 * you may want. In cases where patches are logically incompatible
 * one patch may take precedence over the other as noted in the
 * relevant descriptions.
 */

/* Patches */

/* This patch allows tabbed to handle windows with transparency.
 * You need to uncomment the corresponding line in Makefile to use the -lXrender library
 * when including this patch.
 * https://tools.suckless.org/tabbed/patches/alpha/
 */
#define ALPHA_PATCH 0

/* This patch hides the tab bar if only one tab is open.
 * https://tools.suckless.org/tabbed/patches/autohide/
 */
#define AUTOHIDE_PATCH 0

/* Named after the window title bar patch for dwm this patch divides the
 * tab bar space evenly between the tabbed windows.
 */
#define AWESOMEBAR_PATCH 0

/* This patch allows the height of the bar to be manually specified.
 * https://tools.suckless.org/tabbed/patches/bar-height/
 */
#define BAR_HEIGHT_PATCH 0

/* Only show the basename of the tabbed title; useful with zathura. Toggle with -b.
 * https://tools.suckless.org/tabbed/patches/basenames/
 */
#define BASENAME_PATCH 0

/* This patch moves the tabs / bar to the bottom of the tabbed window.
 * https://github.com/bakkeby/patches/blob/master/tabbed/tabbed-bottomtabs-0.6-20200512-dabf6a2.diff
 */
#define BOTTOM_TABS_PATCH 0

/* This patch centers the tab text.
 * https://www.reddit.com/r/suckless/comments/oi4zjl/tabbed_text_alignment/
 * https://github.com/bakkeby/patches/blob/master/tabbed/tabbed-center-0.6-20200512-dabf6a2.diff
 */
#define CENTER_PATCH 0

/* This patch prints the position number of the client before the window title.
 * https://tools.suckless.org/tabbed/patches/clientnumber/
 */
#define CLIENTNUMBER_PATCH 0

/* Adds support for dragging tabs left and right using the mouse.
 * https://tools.suckless.org/tabbed/patches/drag/
 */
#define DRAG_PATCH 0

/* This patch hides all the tabs and only shows them when Mod+Shift is pressed. All functions
 * with switching, rotating, and creating tabs involve Mod+Shift. When not doing one of these
 * functions, visibility of the tabs is not needed.
 * This patch relies on the keyrelease patch to support show/hide on keypress/keyrelease.
 * https://tools.suckless.org/tabbed/patches/hidetabs/
 */
#define HIDETABS_PATCH 0

/* This patch gives tabbed an icon. This icon is the currently selected tab's icon.
 * If the selected tab has no icon (or no tab is selected), use a (admittedly ugly) default icon.
 *
 * This patch supports both the new EWMH (_NET_WM_ICON) and
 * legacy ICCCM (WM_ICON) ways of setting a window's icon.
 *
 * https://tools.suckless.org/tabbed/patches/icon/
 */
#define ICON_PATCH 0

/* With this patch, handling key input is done with keycodes instead of keysyms making
 * the keyboard layout independent.
 * https://tools.suckless.org/tabbed/patches/keycode/
 */
#define KEYCODE_PATCH 0

/* This patch enables for function handling on KeyRelease events.
 * For example usage see: hidetabs
 * https://tools.suckless.org/tabbed/patches/keyrelease/
 */
#define KEYRELEASE_PATCH 0

/* Add a decorative separator bar to the beginning of each tab.
 * https://tools.suckless.org/tabbed/patches/separator/
 */
#define SEPARATOR_PATCH 0

/* This patch allows tabbed colors to be defined via Xresources.
 * https://tools.suckless.org/tabbed/patches/xresources/
 */
#define XRESOURCES_PATCH 0

/* Expansion of the resources patch above, allows resources to be reloaded during
 * runtime by using a USR1 kill signal.
 *
 * Example use:
 *    pidof tabbed | xargs kill -s USR1
 *
 * https://tools.suckless.org/tabbed/patches/xresources-with-reload-signal/
 */
#define XRESOURCES_RELOAD_PATCH 0

/*
 * Alongside the WM_NAME property, this patch ensures that the WM_CLASS
 * X11 property is set based on the name provided via the -n flag.
 *
 * Without this patch, window rules for tabbed windows may behave
 * unexpectedly in certain window managers, including DWM.
 */
#define CLASS_PATCH 0
