void
motionnotify(const XEvent *e)
{
	const XMotionEvent *ev = &e->xmotion;
	int i, fc;
	Arg arg;

	if (ev->y < 0 || ev->y > bh)
		return;

	if (! (ev->state & Button1Mask)) {
		return;
	}

	if (((fc = getfirsttab()) > 0 && ev->x < TEXTW(before)) || ev->x < 0)
		return;

	if (sel < 0)
		return;

	for (i = fc; i < nclients; i++) {
		if (clients[i]->tabx > ev->x) {
			if (i == sel+1) {
				arg.i = 1;
				movetab(&arg);
			}
			if (i == sel-1) {
				arg.i = -1;
				movetab(&arg);
			}
			break;
		}
	}
}