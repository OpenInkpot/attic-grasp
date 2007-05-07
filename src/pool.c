#include "common.h"

typedef void (*format_fn)(char *path, char *pfx, char *comp, char *pkgname);

static void fmt0_path(char *path, char *pfx, char *comp, char *pkgname)
{
	snprintf(path, PATH_MAX, "%s/pool/%s", CONFIG.output_dir,
			pkgname);
}

static void fmt1_path(char *path, char *pfx, char *comp, char *pkgname)
{
	snprintf(path, PATH_MAX, "%s/pool/%s/%s", CONFIG.output_dir,
			pfx, pkgname);
}

static void fmt2_path(char *path, char *pfx, char *comp, char *pkgname)
{
	snprintf(path, PATH_MAX, "%s/pool/%s/%s/%s", CONFIG.output_dir,
			comp, pfx, pkgname);
}

static format_fn fmtfn[] = { fmt0_path, fmt1_path, fmt2_path };

int pool_mkpath(char *path, char *comp, char *pkgname)
{
	char pfx[5];

	if (!strncmp(pkgname, "lib", 3))
		sprintf(pfx, "lib%c", pkgname[3]);
	else
		sprintf(pfx, "%c", pkgname[0]);

	(fmtfn[2])(path, pfx, comp, pkgname);

	DBG("pool path: %s\n", path);
	return 0;
}

