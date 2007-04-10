#include "common.h"

extern char **environ;

int spawn(char *cmd, char **argv)
{
	pid_t pid;
	int ret;

	pid = fork();
	if (pid) {
		waitpid(-1, &ret, 0);
	} else {
		ret = execve(cmd, argv, environ);
		if (ret) {
			DBG("# exec %s failed\n", cmd);
			exit(EXIT_FAILURE);
		}
	}

	return ret;
}

int mv(char *oldpath, char *newpath)
{
	char *argv[] = { "mv", oldpath, newpath, NULL };
	int ret;

	ret = spawn("/bin/mv", argv);
	if (ret)
		return GE_ERROR;

	return GE_OK;
}

int md5sum(char *file, char *buf)
{
	FILE *p;
	char cmd[FILENAME_MAX];

	if (check_file(file) != GE_OK) return GE_ERROR;
	snprintf(cmd, FILENAME_MAX, "/usr/bin/md5sum %s", file);
	p = popen(cmd, "r");
	if (!p)
		return GE_ERROR;

	fscanf(p, "%32s", buf);
	pclose(p);

	return 0;
}

void write_gbp(char *pkgname)
{
	FILE *gbp;
	char gbpfile_name[FILENAME_MAX];
	int i;

	snprintf(gbpfile_name, FILENAME_MAX, "%s/%s/.git/gbp.conf",
			CONFIG.gitrepos_dir, pkgname);
	SAY("Writing %s\n", gbpfile_name);
	gbp = fopen(gbpfile_name, "w");
	/* if (!gbp) */
	for (i = 0; i < GRASP.ntarballs; i++) {
		if (!strcmp(GRASP.branch[i], "master"))
			fputs("[DEFAULT]\n", gbp);
		else
			fprintf(gbp, "[branch:%s]\n", GRASP.branch[i]);

		fprintf(gbp, "origtargz = %s\n", GRASP.tarball_path[i]);
	}
	fclose(gbp);
}

int git_clone(char *url)
{
	char *argv[] = { "git", "clone", url, NULL, NULL };
	char git_dir[FILENAME_MAX];
	int ret;

	snprintf(git_dir, FILENAME_MAX, "%s/%s", CONFIG.gitrepos_dir,
			GRASP.pkgname);
	argv[3] = git_dir;
	ret = spawn("/usr/bin/git", argv);
	write_gbp(GRASP.pkgname);

	return ret;
}

int git_pull(char *url, char *branch)
{
	char *argv[] = { "git", "pull", url, branch, NULL };
	char git_dir[FILENAME_MAX];
	int ret;

	snprintf(git_dir, FILENAME_MAX, "%s/%s/.git", CONFIG.gitrepos_dir,
			GRASP.pkgname);
	setenv("GIT_DIR", git_dir, 1);
	ret = spawn("/usr/bin/git", argv);
	write_gbp(GRASP.pkgname);
	unsetenv("GIT_DIR");

	return ret;
}

int git_checkout(char *branch)
{
	char *argv[] = { "git", "checkout", branch, NULL };
	char git_dir[FILENAME_MAX];
	int ret;

	snprintf(git_dir, FILENAME_MAX, "%s/%s", CONFIG.gitrepos_dir,
			GRASP.pkgname);
	chdir(git_dir);
	ret = spawn("/usr/bin/git", argv);
	chdir(PWD);

	return ret;
}

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

int git_buildpackage(char *branch)
{
	char *argv[] = { "git-buildpackage", "-uc", "-us", "-rfakeroot",
		"-d", "-S", NULL, NULL };
	char git_dir[FILENAME_MAX];
	int ret;

	/* make sure we're on a proper branch */
	if (branch && strcmp(branch, "master")) {
		ret = git_checkout(branch);
		if (ret)
			return GE_ERROR;

		asprintf(&argv[6], "--git-debian-branch=%s", branch);
	} else {
		ret = git_checkout("master");
		if (ret)
			return GE_ERROR;
	}

	snprintf(git_dir, FILENAME_MAX, "%s/%s", CONFIG.gitrepos_dir,
			GRASP.pkgname);
	chdir(git_dir);
	ret = spawn("/usr/bin/git-buildpackage", argv);
	chdir(PWD);

	if (argv[6])
		free(argv[6]);

	return ret;
}

