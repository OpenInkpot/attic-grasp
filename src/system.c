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

int tar_zxf(char *tarball, char *dir)
{
	char *argv[] = { "tar", "vzxf", tarball, "-C", dir, NULL };
	int ret;

	ret = spawn("/bin/tar", argv);
	if (ret)
		return GE_ERROR;

	return GE_OK;
}

int rm_rf(char *dir)
{
	char *argv[] = { "rm", "-rf", dir, NULL };
	int ret;

	ret = spawn("/bin/rm", argv);
	if (ret)
		return GE_ERROR;

	return GE_OK;
}

int mkdir_p(char *dst, mode_t mode)
{
	char *argv[] = { "mkdir", "-p", dst, NULL };
	int ret;

	ret = spawn("/bin/mkdir", argv);
	if (ret)
		return GE_ERROR;

	return GE_OK;
}

int copy(char *src, char *dst)
{
	char *argv[] = { "cp", src, dst, NULL };
	int ret;

	ret = spawn("/bin/cp", argv);
	if (ret)
		return GE_ERROR;

	return GE_OK;
}

int link_or_copy(char *src, char *dst)
{
	int ret;

	ret = link(src, dst);
	if (ret) /* in case of cross-device link attempt */
		ret = copy(src, dst);

	return (ret ? GE_ERROR : GE_OK);
}

int md5sum(char *file, char *buf)
{
	FILE *p;
	char cmd[PATH_MAX];

	if (check_file(file) != GE_OK) return GE_ERROR;
	snprintf(cmd, PATH_MAX, "/usr/bin/md5sum %s", file);
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
	char gbpfile_name[PATH_MAX];
	int i;

	snprintf(gbpfile_name, PATH_MAX, "%s/%s/.git/gbp.conf",
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
	char git_dir[PATH_MAX];
	int ret;

	snprintf(git_dir, PATH_MAX, "%s/%s", CONFIG.gitrepos_dir,
			GRASP.pkgname);
	argv[3] = git_dir;
	ret = spawn("/usr/bin/git", argv);
	write_gbp(GRASP.pkgname);

	return ret;
}

int git_pull(char *url, char *branch)
{
	char *argv[] = { "git", "pull", url, branch, NULL };
	char git_dir[PATH_MAX];
	int ret;

	snprintf(git_dir, PATH_MAX, "%s/%s/.git", CONFIG.gitrepos_dir,
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
	char git_dir[PATH_MAX];
	int ret;

	snprintf(git_dir, PATH_MAX, "%s/%s", CONFIG.gitrepos_dir,
			GRASP.pkgname);
	chdir(git_dir);
	ret = spawn("/usr/bin/git", argv);
	chdir(PWD);

	return ret;
}

int dpkg_source(char *dir, char *where)
{
	char *argv[] = { "dpkg-source", "-b", dir, NULL };
	int ret;

	chdir(where);
	ret = spawn("/usr/bin/dpkg-source", argv);
	chdir(PWD);

	return ret;
}

int git_export(char *branch, char *dir)
{
	char git_dir[PATH_MAX];
	char cmd[1024];
	int ret;

	snprintf(git_dir, PATH_MAX, "%s/%s/.git", CONFIG.gitrepos_dir,
			GRASP.pkgname);

	snprintf(cmd, 1024, "/usr/bin/git-archive --format=tar %s | "
			"( cd %s && tar xf - )", branch, dir);
	setenv("GIT_DIR", git_dir, 1);
	ret = system(cmd);
	unsetenv("GIT_DIR");

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

int find_first_dir(char *where, char **res)
{
	int ret;
	DIR *tmp;
	struct dirent *de;
	struct stat statbuf;

	*res = NULL;
	tmp = opendir(where);
	do {
		de = readdir(tmp);
		if (!de)
			break;
		
		if (
			de->d_name[0] == '.' && 
			((de->d_name[1] == '.' && de->d_name[2] == '\0') ||
			(de->d_name[1] == '\0'))
		   )
			continue;

		/* XXX: unsafe */
		asprintf(res, "%s/%s", where, de->d_name);
		ret = lstat(*res, &statbuf);

		if (!ret && S_ISDIR(statbuf.st_mode))
			break;

		*res = NULL;
	} while (de);
	closedir(tmp);

	return (*res ? GE_OK : GE_ERROR);
}

int git_buildpackage(int nbranch)
{
	int ret;
	char tmpdir[PATH_MAX], tarball_dst[PATH_MAX];
	char *tmpdir_r, *fulldir, *tarball_file;
	char *branch = GRASP.branch[nbranch];

	/* make sure we're on a proper branch */
	if (branch && strcmp(branch, "master")) {
		ret = git_checkout(branch);
		if (ret)
			return GE_ERROR;
	} else {
		ret = git_checkout("master");
		if (ret)
			return GE_ERROR;
	}

	snprintf(tmpdir, PATH_MAX, "%s/grasp_XXXXXX",
			GRASP.pkg_out_dir);
	tmpdir_r = mkdtemp(tmpdir);
	if (!tmpdir_r) {
		SHOUT("Failed to create temporary directory %s\n", tmpdir);
		return GE_ERROR;
	}
	
	/* unpack .orig.tar.gz if it's not a native package */
	if (GRASP.ntarballs) {
		ret = check_file(GRASP.tarball_path[nbranch]);
		if (ret != GE_OK) {
			SHOUT("Error: tarball file %s is missing. "
					"Try 'update' command first\n",
					GRASP.tarball_path[nbranch]);
			return GE_ERROR;
		}

		tar_zxf(GRASP.tarball_path[nbranch], tmpdir);

		find_first_dir(tmpdir, &fulldir);
		if (!fulldir) {
			ret = GE_ERROR;
			goto out_rm;
		}
	} else {
		asprintf(&fulldir, "%s/%s", tmpdir, GRASP.pkgname);
		mkdir_p(fulldir, 0755);
	}

	git_export(branch, fulldir);

	if (GRASP.ntarballs) {
		tarball_file = strrchr(GRASP.tarball_url[nbranch], '/');
		if (!tarball_file)
			tarball_file = GRASP.tarball_url[nbranch];

		snprintf(tarball_dst, PATH_MAX, "%s/%s", GRASP.pkg_out_dir,
				tarball_file);

		ret = check_file(tarball_dst);
		if (ret == GE_NOENT) {
			ret = link_or_copy(GRASP.tarball_path[nbranch],
					tarball_dst);
			if (ret != GE_OK) {
				SHOUT("Error: cannot link or copy %s to %s.\n",
						GRASP.tarball_path[nbranch],
						tarball_dst);
				return GE_ERROR;
			}
		}
	}

	ret = dpkg_source(fulldir, GRASP.pkg_out_dir);
	if (ret != GE_OK) {
		SHOUT("Error: source package build failed [dpkg-source].\n");
		return GE_ERROR;
	}

	xfree(fulldir);
out_rm:
	rm_rf(tmpdir);
	return ret;
}

