#include "common.h"

/*
 * Take all necessary preparation and validation steps
 * before proceeding to commands
 *
 * 1) fill in necessary paths in GRASP
 * 2) check for validity of paths:
 *    * local copy of git repo for 'pkgname'
 *    * directory one level above (gitrepos_dir)
 *    * directory that hosts upstream tarballs (tarballs_dir)
 * 3) gitrepos_dir and tarballs_dir are created if missing
 */
int pkg_cmd_prologue(char *pkgname)
{
	int r, i;
	int ret = GS_OK;
	char *p;

	/* fill in paths and urls in GRASP */
	/* -- package name */
	strncpy(GRASP.pkgname, pkgname, PKGNAME_MAX);

	/* -- path to local git repo */
	snprintf(GRASP.gitrepo_path, PATH_MAX, "%s/%s", CONFIG.gitrepos_dir,
			GRASP.pkgname);

	/* check if local copy already exists */
	r = check_dir(GRASP.gitrepo_path);

	if (r == GE_OK) {
		/* it exists, which is ok */
		/* this case is valid for "update" command */
		SAY("Path %s exists, will act as 'update'\n",
				GRASP.gitrepo_path);
	} else if (r == GE_NOENT) {
		/* it doesn't exist, which is ok: git will create it for us */
		/* this case is valid for "getpkg" command */
		SAY("Path %s doesn't exist, will act as 'getpkg'\n",
				GRASP.gitrepo_path);

		ret = GS_NOLOCALCOPY;
	} else {
		/* it isn't accessible */
		SHOUT("Error: Path %s already exists and is inaccessible\n"
			"cannot proceed from here.\n", GRASP.gitrepo_path);

		return GE_ERROR;
	}

	/* check if gitrepos_dir exists */
	if (ret & GS_NOLOCALCOPY) {
		r = check_and_create_dir(CONFIG.gitrepos_dir);

		if (r != GE_OK) {
			SHOUT("Error: failed to create %s.\n"
					"cannot proceed from here.\n",
					CONFIG.gitrepos_dir);

			return GE_ERROR;
		}
	}

	/* check if tarballs_dir exists */
	r = check_and_create_dir(CONFIG.tarballs_dir);

	if (r != GE_OK) {
		SHOUT("Error: failed to create %s.\n"
				"cannot proceed from here.\n",
				CONFIG.tarballs_dir);

		return GE_ERROR;
	}

	r = get_grasp(GRASP.pkgname);
	if (r)
		return GE_ERROR;

	for (i = 0; i < GRASP.ntarballs; i++) {
		/* strip tarball name from tarball_url */
		p = strrchr(GRASP.tarball_url[i], '/');
		if (!p) p = GRASP.tarball_url[i];
		p++;

		snprintf(GRASP.tarball_path[i], PATH_MAX, "%s/%s",
				CONFIG.tarballs_dir, p);
		DBG("tarball_path[%d]: %s\n", i, GRASP.tarball_path[i]);
	}

	return ret;
}

int pkg_cmd_epilogue()
{
	char newpath[PATH_MAX];
	int ret;

	if (GRASP.move_grasp) {
		snprintf(newpath, PATH_MAX, "%s/%s/.git/grasp",
				CONFIG.gitrepos_dir, GRASP.pkgname);
		ret = mv(GRASP.graspfile_local, newpath);
		/*ret = put_grasp(newpath);*/
		if (ret) {
			SHOUT("Can't put grasp file to %s\n", newpath);
			return GE_ERROR;
		}
	}

	free_grasp();

	return GE_OK;
}

