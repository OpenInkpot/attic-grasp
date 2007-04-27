#include "common.h"

/* <URL>/<pkg>.git/grasp */
static int grasp_config;

grasp_config_t GRASP;

/*
 * Download and validate remote grasp file
 */
int get_grasp()
{
	char graspfile_url[URL_MAX];
	int c;

	/* compute grasp file URL */
	snprintf(graspfile_url, URL_MAX, "%s/%s.git/grasp",
			CONFIG.gitbase_url, GRASP.pkgname);

	/* check if local git repo exists at this point */
	snprintf(GRASP.graspfile_local, PATH_MAX, "%s/%s/.git",
			CONFIG.gitrepos_dir, GRASP.pkgname);
	c = check_dir(GRASP.graspfile_local);

	if (c != GE_OK) {
		DBG("local git repo doesn't exist, will save grasp to /tmp\n");
		snprintf(GRASP.graspfile_local, PATH_MAX, "/tmp/%s.grasp",
				GRASP.pkgname);
		GRASP.move_grasp = 1;
	} else {
		snprintf(GRASP.graspfile_local, PATH_MAX, "%s/%s/.git/grasp",
				CONFIG.gitrepos_dir, GRASP.pkgname);
		c = check_file(GRASP.graspfile_local);
		if (c == GE_OK)
			c = strncasecmp(CONFIG.reget_grasp, "no", 3);
		GRASP.move_grasp = 0;
	}

	if (c != GE_OK)
		http_get_file(graspfile_url, GRASP.graspfile_local);

	grasp_config = config_read(GRASP.graspfile_local);
	if (grasp_config < 0) {
		SHOUT("No grasp file found, expected: %s\n",
				GRASP.graspfile_local);
		return -ENOENT;
	}

	/* validate the grasp config file */
	CONFIG_VALIDATE_OPT(GRASP.format, grasp_config, "format");

	/* minimal number of tarballs is 0 (for native packages) */
	GRASP.ntarballs = config_get_item_values(grasp_config, "tarball_url");
	GRASP.tarball_url = xmalloc(GRASP.ntarballs * sizeof(char *));
	GRASP.tarball_md5 = xmalloc(GRASP.ntarballs * sizeof(char *));
	GRASP.tarball_path = xmalloc(GRASP.ntarballs * sizeof(char *));
	for (c = 0; c < GRASP.ntarballs; c++)
		GRASP.tarball_path[c] = xmalloc(PATH_MAX);

	CONFIG_VALIDATE_OPTM(GRASP.tarball_url, grasp_config, "tarball_url",
			GRASP.ntarballs);
	CONFIG_VALIDATE_OPTN(GRASP.tarball_md5, grasp_config, "tarball_md5",
			GRASP.ntarballs);

	/* minimal number of branches is 1 (master) */
	GRASP.nbranches = config_get_item_values(grasp_config, "branch");
	if (GRASP.nbranches < 1) {
		SHOUT("GRASP provides no branches.\n");
		return GE_ERROR;
	}

	GRASP.branch = xmalloc(GRASP.nbranches * sizeof(char *));
	CONFIG_VALIDATE_OPTM(GRASP.branch, grasp_config, "branch",
			GRASP.nbranches);
	if (GRASP.nbranches < GRASP.ntarballs) {
		SHOUT("GRASP file specifies more tarballs than branches.\n");
		return GE_ERROR;
	}

	return 0;
}

void free_grasp()
{
	int c;

	xfree(GRASP.tarball_url);
	xfree(GRASP.tarball_md5);
	for (c = 0; c < GRASP.ntarballs; c++)
		xfree(GRASP.tarball_path[c]);
	xfree(GRASP.tarball_path);

	config_dispose(grasp_config);
}

int put_grasp(char *filename)
{
	return config_write(grasp_config, filename);
}

