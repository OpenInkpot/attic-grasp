#include "common.h"

#define GIT_VERSION(a, b, c, d) \
	(((a) << 24) | ((b) << 16) | ((c) << 8) | (d))

/* compatibility flags */
#define GIT_CLONES_ALL (1 << 0)

static unsigned int compat = 0;

/*
 * return bitwise or'ed git version:
 * needed for run-time git compatibility detection
 */
static int get_git_version()
{
	FILE *pipe;
	char cmd[PATH_MAX];
	char *git, *version, *ver_str, *p, *q;
	int ret = 0;

	snprintf(cmd, PATH_MAX, "%s --version", GIT_BIN_PATH);
	pipe = popen(cmd, "r");
	if (!pipe)
		return GE_ERROR;

	fscanf(pipe, "%as %as %as", &git, &version, &ver_str);
	pclose(pipe);

	q = ver_str;
	p = strchr(q, '.');
	ret = strtoul(q, NULL, 10) << 24;
	if (!p)
		return ret;

	q = p + 1;
	p = strchr(q, '.');
	ret |= strtoul(q, NULL, 10) << 16;
	if (!p)
		return ret;

	q = p + 1;
	p = strchr(q, '.');
	ret |= strtoul(q, NULL, 10) << 8;
	if (!p)
		return ret;

	q = p + 1;
	p = strchr(q, '.');
	ret |= strtoul(q, NULL, 10);

	/* there may be as many minor-minor version numbers as we can never
	 * stick into one integer */
	return ret;
}

/*
 * this is here for us to know what kind of git are we talking to,
 * to select appropriate compatibility flags
 *
 * note: some version codes might need to be more precise; I'm too
 *       lazy to sniff through git lists/changelogs
 */
int git_init()
{
	int git_version;

	git_version = get_git_version();

	/* git-clone behaves differently on 1.4 and 1.5:
	 * the former one fetches all remote branches, while
	 * the latter one only fetches 'master', which is not
	 * what we want */
	if (git_version < GIT_VERSION(1,5,0,0))
		compat |= GIT_CLONES_ALL;
}

/*
 * fetch remote branch 'branch' to a local branch with the same name
 * from remote repository 'url'
 */
int git_pull(char *url, char *branch)
{
	char *argv[] = { "git", "pull", url, NULL, NULL };
	char git_dir[PATH_MAX];
	char refspec[REFSPEC_MAX];
	int ret;

	/* calculate working copy directory */
	snprintf(git_dir, PATH_MAX, "%s/%s", CONFIG.gitrepos_dir,
			GRASP.pkgname);

	/* calculate refspec */
	snprintf(refspec, REFSPEC_MAX, "%s:%s", branch, branch);
	argv[3] = refspec;

	chdir(git_dir);
	ret = spawn(GIT_BIN_PATH, argv);
	write_gbp(GRASP.pkgname);
	chdir(PWD);

	return ret;
}

/*
 * clone remote git repo 'url' locally;
 * will fetch all _relevant_ remote branches (those
 * mentioned in remote graspfile)
 */
int git_clone(char *url)
{
	char *argv[] = { "git", "clone", url, NULL, NULL };
	char git_dir[PATH_MAX];
	int ret, i;

	/* calculate path to local repository */
	snprintf(git_dir, PATH_MAX, "%s/%s", CONFIG.gitrepos_dir,
			GRASP.pkgname);
	argv[3] = git_dir;

	ret = spawn(GIT_BIN_PATH, argv);
	if (ret != GE_OK)
		return ret;

	write_gbp(GRASP.pkgname);

	/* compatibility: GIT_CLONES_ALL:
	 * emulate old git-clone behavior on newer gits */
	if (!(compat & GIT_CLONES_ALL) && (GRASP.nbranches > 1)) {
		DBG("git-clone doesn't fetch branches, fetching manually\n");

		for (i = 0; i < GRASP.nbranches; i++) {
			if (!strcmp(GRASP.branch[i], "master"))
				continue;

			ret = git_pull(url, GRASP.branch[i]);
			if (ret != GE_OK) {
				SHOUT("Failed to fetch branch %s\n",
						GRASP.branch[i]);
				return ret;
			}
		}
	}

	return ret;
}

/*
 * checkout branch named 'branch' to a local copy
 * note to self: forced checkout wouldn't hurt here, would it?
 */
int git_checkout(char *branch)
{
	char *argv[] = { "git", "checkout", "-f", branch, NULL };
	char git_dir[PATH_MAX];
	int ret;

	snprintf(git_dir, PATH_MAX, "%s/%s", CONFIG.gitrepos_dir,
			GRASP.pkgname);
	chdir(git_dir);
	ret = spawn(GIT_BIN_PATH, argv);
	chdir(PWD);

	return ret;
}

/*
 * export a branch from local repository into a given directory
 */
int git_export(char *branch, char *dir)
{
	char git_dir[PATH_MAX];
	char cmd[1024];
	int ret;

	snprintf(git_dir, PATH_MAX, "%s/%s/.git", CONFIG.gitrepos_dir,
			GRASP.pkgname);

	snprintf(cmd, 1024, "%s archive --format=tar %s | "
			"( cd %s && tar xf - )", GIT_BIN_PATH, branch, dir);
	setenv("GIT_DIR", git_dir, 1);
	ret = system(cmd);
	unsetenv("GIT_DIR");

	return ret;
}

/*
 * update all (relevant) branches in a local repository
 */
int git_pull_all(char *url)
{
	int ret = 0;
	int i;

	for (i = 0; i < GRASP.nbranches; i++) {
		ret = git_checkout(GRASP.branch[i]);
		if (ret) return GE_ERROR;

		ret = git_pull(url, GRASP.branch[i]);
		if (ret) return GE_ERROR;
	}

	return ret;
}

