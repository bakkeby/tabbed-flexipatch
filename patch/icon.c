static unsigned long icon[ICON_WIDTH * ICON_HEIGHT + 2];

void
xseticon(void)
{
	Atom ret_type;
	XWMHints *wmh, *cwmh;
	int ret_format;
	unsigned long ret_nitems, ret_nleft;
	long offset = 0L;
	unsigned char *data;

	wmh = XGetWMHints(dpy, win);
	wmh->flags &= ~(IconPixmapHint | IconMaskHint);
	wmh->icon_pixmap = wmh->icon_mask = None;


	if (XGetWindowProperty(dpy, clients[sel]->win, wmatom[WMIcon], offset, LONG_MAX, False,
	                       XA_CARDINAL, &ret_type, &ret_format, &ret_nitems,
	                       &ret_nleft, &data) == Success &&
	    ret_type == XA_CARDINAL && ret_format == 32)
	{
		XChangeProperty(dpy, win, wmatom[WMIcon], XA_CARDINAL, 32,
		                PropModeReplace, data, ret_nitems);
	} else if ((cwmh = XGetWMHints(dpy, clients[sel]->win)) && cwmh->flags & IconPixmapHint) {
		XDeleteProperty(dpy, win, wmatom[WMIcon]);
		wmh->flags |= IconPixmapHint;
		wmh->icon_pixmap = cwmh->icon_pixmap;
		if (cwmh->flags & IconMaskHint) {
			wmh->flags |= IconMaskHint;
			wmh->icon_mask = cwmh->icon_mask;
		}
		XFree(cwmh);
	} else {
		XChangeProperty(dpy, win, wmatom[WMIcon], XA_CARDINAL, 32,
		                PropModeReplace, (unsigned char *) icon, ICON_WIDTH * ICON_HEIGHT + 2);
	}
	XSetWMHints(dpy, win, wmh);
	XFree(wmh);
	XFree(data);
}