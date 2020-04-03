void
keyrelease(const XEvent *e)
{
	const XKeyEvent *ev = &e->xkey;
	unsigned int i;
	KeySym keysym;

	keysym = XkbKeycodeToKeysym(dpy, (KeyCode)ev->keycode, 0, 0);
	for (i = 0; i < LENGTH(keyreleases); i++) {
		if (keysym == keyreleases[i].keysym &&
		    CLEANMASK(keyreleases[i].mod) == CLEANMASK(ev->state) &&
		    keyreleases[i].func)
			keyreleases[i].func(&(keyreleases[i].arg));
	}
}