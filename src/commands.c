#include "common.h"

#define URL_MAX 128

/*
 * Build a source package for a given branch
 * TODO: data -> branch name
 */
int cmd_build(void *data)
{
	int ret, i;

	for (i = 0; i < GRASP.nbranches; i++) {
		ret = git_buildpackage(GRASP.branch[i]);
		if (ret != GE_OK)
			return ret;
	}

	return 0;
}

/*
 * Update local repository and tarballs for package
 * data: branch name (do all branches if unspecified)
 */
int cmd_update(void *data)
{
	int ret, i;
	char git_url[URL_MAX];
	char tarball_md5[MD5_MAX + 1];
	char *branch = (char *)data;

	for (i = 0; i < GRASP.ntarballs; i++) {
		tarball_md5[0] = tarball_md5[MD5_MAX] = '\0';
		ret = md5sum(GRASP.tarball_path[i], tarball_md5);

		if (!ret)
			DBG("[md5] %s: %s (local), %s (remote)\n",
					GRASP.tarball_path[i], tarball_md5,
					GRASP.tarball_md5[i]);
		if (ret ||
			strncmp(tarball_md5, GRASP.tarball_md5[i], MD5_MAX)) {
			DBG("md5 didn't match, redownloading\n");
			SAY("Downloading source tarball from %s...\n",
					GRASP.tarball_url[i]);
			http_get_file(GRASP.tarball_url[i],
					GRASP.tarball_path[i]);
		}
	}

	snprintf(git_url, URL_MAX, "%s/%s.git", CONFIG.gitbase_url,
			GRASP.pkgname);

	if (branch) {
		SAY("Doing git pull %s [%s]\n", git_url, branch);
		ret = git_pull(git_url, branch);
	} else
		ret = git_pull_all(git_url);

	return (ret ? GE_ERROR : GE_OK);
}

/*
 * Clone a git repo and download all relevant tarballs
 * data: branch name (see above).
 */
int cmd_getpkg(void *data)
{
	int ret, i;
	char git_url[URL_MAX];
	char tarball_md5[MD5_MAX + 1];

	for (i = 0; i < GRASP.ntarballs; i++) {
		tarball_md5[0] = tarball_md5[MD5_MAX] = '\0';
		ret = md5sum(GRASP.tarball_path[i], tarball_md5);

		if (!ret)
			DBG("[md5] %s: %s (local), %s (remote)\n",
					GRASP.tarball_path[i], tarball_md5,
					GRASP.tarball_md5[i]);
		if (ret ||
			strncmp(tarball_md5, GRASP.tarball_md5[i], MD5_MAX)) {
			DBG("md5 didn't match, redownloading\n");
			SAY("Downloading source tarball from %s...\n",
					GRASP.tarball_url[i]);
			http_get_file(GRASP.tarball_url[i],
					GRASP.tarball_path[i]);
		}
	}

	snprintf(git_url, URL_MAX, "%s/%s.git", CONFIG.gitbase_url,
			GRASP.pkgname);
	SAY("Doing git clone %s\n", git_url);
	git_clone(git_url);

	return GE_OK;
}

