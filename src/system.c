#include "common.h"

extern char **environ;

/*
 * execute a thing (fork, exec, wait)
 */
int spawn(char *cmd, char **argv)
{
	pid_t pid;
	int i = 0;
	int ret;

	if (verbosity >= VERB_DEBUG) {
		DBG("going to execute:");
		while (argv[i])
			output(ERR, VERB_DEBUG, " %s", argv[i++]);
		output(ERR, VERB_DEBUG, "\n");
	}

	pid = fork();
	if (pid) {
		waitpid(-1, &ret, 0);
	} else {
		ret = execve(cmd, argv, environ);
		if (ret) {
			DBG("exec %s failed\n", cmd);
			exit(EXIT_FAILURE);
		}
	}

	return ret;
}

/*
 * move a thing from 'oldpath' to a 'newpath'
 */
int mv(char *oldpath, char *newpath)
{
	char *argv[] = { "mv", oldpath, newpath, NULL };
	int ret;

	ret = spawn(MV_BIN_PATH, argv);
	if (ret)
		return GE_ERROR;

	return GE_OK;
}

/*
 * unpack a tarball to a given directory
 */
int tar_zxf(char *tarball, char *dir)
{
	char *argv[] = { "tar", "vzxf", tarball, "-C", dir, NULL };
	int ret;

	ret = spawn(TAR_BIN_PATH, argv);
	if (ret)
		return GE_ERROR;

	return GE_OK;
}

/*
 * remove a directory (with all contents)
 */
int rm_rf(char *dir)
{
	char *argv[] = { "rm", "-rf", dir, NULL };
	int ret;

	ret = spawn(RM_BIN_PATH, argv);
	if (ret)
		return GE_ERROR;

	return GE_OK;
}

/*
 * create a directory (with all the missing parent directories)
 */
int mkdir_p(char *dst, mode_t mode)
{
	char *argv[] = { "mkdir", "-p", dst, NULL };
	int ret;

	ret = spawn(MKDIR_BIN_PATH, argv);
	if (ret)
		return GE_ERROR;

	return GE_OK;
}

/*
 * copy something from 'src' to 'dst'
 */
int copy(char *src, char *dst)
{
	char *argv[] = { "cp", src, dst, NULL };
	int ret;

	ret = spawn(CP_BIN_PATH, argv);
	if (ret)
		return GE_ERROR;

	return GE_OK;
}

/*
 * make 'dst' a hardlink to 'src', if possible, copy otherwise
 */
int link_or_copy(char *src, char *dst)
{
	int ret;

	ret = link(src, dst);
	if (ret) /* in case of cross-device link attempt */
		ret = copy(src, dst);

	return (ret ? GE_ERROR : GE_OK);
}

/*
 * calculate md5 hash of a 'file'
 */
int md5sum(char *file, char *buf)
{
	FILE *p;
	char cmd[PATH_MAX];

	if (check_file(file) != GE_OK) return GE_ERROR;
	snprintf(cmd, PATH_MAX, "%s %s", MD5SUM_BIN_PATH, file);
	p = popen(cmd, "r");
	if (!p)
		return GE_ERROR;

	fscanf(p, "%32s", buf);
	pclose(p);

	return 0;
}

/*
 * write gbp.conf to a local git repository
 */
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

/*
 * call 'dpkg-source' to build a source package
 */
int dpkg_source(char *dir, char *where)
{
	char *argv[] = { "dpkg-source", "-b", dir, NULL };
	int ret;

	chdir(where);
	ret = spawn("/usr/bin/dpkg-source", argv);
	chdir(PWD);

	return ret;
}

/*
 * find a first directory in a given directory
 * (a lame way to detect how a directory inside a tarball was named)
 */
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

/*
 * build a source package of n'th branch of a local package's repo
 * (and put it to output_dir)
 */
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

