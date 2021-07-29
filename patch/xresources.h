#include <X11/Xresource.h>

/* Xresources preferences */
enum resource_type {
	STRING = 0,
	INTEGER = 1,
	FLOAT = 2
};

typedef struct {
	char *name;
	enum resource_type type;
	void *dst;
} ResourcePref;

static void config_init(void);
static int resource_load(XrmDatabase db, char *name, enum resource_type rtype, void *dst);