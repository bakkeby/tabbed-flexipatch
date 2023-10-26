Similar to [dwm-flexipatch](https://github.com/bakkeby/dwm-flexipatch) this tabbed 0.7 (5ddbc73, 2022-10-05) project has a different take on patching. It uses preprocessor directives to decide whether or not to include a patch during build time. Essentially this means that this build, for better or worse, contains both the patched _and_ the original code. The aim being that you can select which patches to include and the build will contain that code and nothing more.

For example to include the `alpha` patch then you would only need to flip this setting from 0 to 1 in [patches.h](https://github.com/bakkeby/tabbed-flexipatch/blob/master/patches.def.h):
```c
#define ALPHA_PATCH 1
```

Once you have found out what works for you and what doesn't then you should be in a better position to choose patches should you want to start patching from scratch.

Alternatively if you have found the patches you want, but don't want the rest of the flexipatch entanglement on your plate then you may want to have a look at [flexipatch-finalizer](https://github.com/bakkeby/flexipatch-finalizer); a custom pre-processor tool that removes all the unused flexipatch code leaving you with a build that contains the patches you selected.

Refer to [https://tools.suckless.org/tabbed/](https://tools.suckless.org/tabbed/) for details on tabbed, how to install it and how it works.

---

### Changelog:

2023-10-26 - Added the separator, drag, basenames and move-clamped patches

2022-03-14 - Added the awesomebar patch

2021-07-29 - Added the bar-height and xresources patches

2021-07-26 - Added the center patch and the bottom tabs patch

2020-09-11 - Added icon patch

2020-04-03 - Added alpha, autohide, clientnumber, hidetabs, keycode and keyrelease patches

### Patches included:

   - [alpha](https://tools.suckless.org/tabbed/patches/alpha/)
      - the alpha patch allows tabbed to handle windows with transparency

   - [autohide](https://tools.suckless.org/tabbed/patches/autohide/)
      - hides the tab bar if only one tab is open

   - awesomebar
      - evenly divides tab bar space between the tabbed windows

   - [bar-height](https://tools.suckless.org/tabbed/patches/bar-height/)
      - allows the height of the bar to be manually specified

   - [basenames](https://tools.suckless.org/tabbed/patches/basenames/)
      - show only the basename of the tabbed title

   - [bottomtabs](https://github.com/bakkeby/patches/blob/master/tabbed/tabbed-bottomtabs-0.6-20200512-dabf6a2.diff)
      - moves the tabs / bar to the bottom of the tabbed window

   - [center](https://github.com/bakkeby/patches/blob/master/tabbed/tabbed-center-0.6-20200512-dabf6a2.diff)
      - centers window titles in tabs

   - [clientnumber](https://tools.suckless.org/tabbed/patches/clientnumber/)
      - prints the position number of the client before the window title

   - [drag](https://tools.suckless.org/tabbed/patches/drag/)
      - adds support for dragging tabs left and right using the mouse

   - [hidetabs](https://tools.suckless.org/tabbed/patches/hidetabs/)
      - this patch hides all the tabs and only shows them when Mod+Shift is pressed

   - [icon](https://tools.suckless.org/tabbed/patches/icon/)
      - this patch gives tabbed an icon
      - the icon is the currently selected tab's icon
      - if the selected tab has no icon (or no tab is selected), use a default icon

   - [keycode](https://tools.suckless.org/tabbed/patches/keycode/)
      - with this patch, handling key input is done with keycodes instead of keysyms making the keyboard layout independent

   - [keyrelease](https://tools.suckless.org/tabbed/patches/keyrelease/)
      - this patch enables for function handling on KeyRelease events

   - [move-clamped](https://tools.suckless.org/tabbed/patches/move-clamped/)
      - makes tabbed interpret large position numbers as the largest known position
      - allows for `Ctrl-9`, for example, to always select the rightmost tab, even if there are only
        4 tabs

   - [separator](https://tools.suckless.org/tabbed/patches/separator/)
      - adds a decorator separator to the beginning of each tab

   - [xresources](https://tools.suckless.org/tabbed/patches/xresources/)
      - allows tabbed colors to be defined via Xresources
