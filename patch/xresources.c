void
config_init(void)
{
	char *resm;
	XrmDatabase db;
	ResourcePref *p;
	Display *dpy;

	if(!(dpy = XOpenDisplay(NULL)))
		die("Can't open display\n");

	XrmInitialize();
	resm = XResourceManagerString(dpy);
	if (!resm)
		return;

	db = XrmGetStringDatabase(resm);
	for (p = resources; p < resources + LENGTH(resources); p++)
		resource_load(db, p->name, p->type, p->dst);
}

int
resource_load(XrmDatabase db, char *name, enum resource_type rtype, void *dst)
{
	char **sdst = dst;
	int *idst = dst;
	float *fdst = dst;

	char fullname[256];
	char fullclass[256];
	char *type;
	XrmValue ret;

	snprintf(fullname, sizeof(fullname), "%s.%s", "tabbed", name);
	snprintf(fullclass, sizeof(fullclass), "%s.%s", "tabbed", name);
	fullname[sizeof(fullname) - 1] = fullclass[sizeof(fullclass) - 1] = '\0';

	XrmGetResource(db, fullname, fullclass, &type, &ret);
	if (ret.addr == NULL || strncmp("String", type, 64))
		return 1;

	switch (rtype) {
	case STRING:
		*sdst = ret.addr;
		break;
	case INTEGER:
		*idst = strtoul(ret.addr, NULL, 10);
		break;
	case FLOAT:
		*fdst = strtof(ret.addr, NULL);
		break;
	}
	return 0;
}

#if XRESOURCES_RELOAD_PATCH
static int colors_changed = 0;

void
xrdb_reload(int sig)
{
	config_init();
	colors_changed = 1;
	signal(SIGUSR1, xrdb_reload);
}

void
writecolors(void)
{
	dc.norm[ColBG] = getcolor(normbgcolor);
	dc.norm[ColFG] = getcolor(normfgcolor);
	dc.sel[ColBG] = getcolor(selbgcolor);
	dc.sel[ColFG] = getcolor(selfgcolor);
	dc.urg[ColBG] = getcolor(urgbgcolor);
	dc.urg[ColFG] = getcolor(urgfgcolor);

	colors_changed = 0;
}
#endif // XRESOURCES_RELOAD_PATCH
