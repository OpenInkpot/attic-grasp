/*
 * config.h
 * 
 */

#ifndef   __CONFIG_H__
#define   __CONFIG_H__

#define CONFIG_LINE_LENGTH	256
#define CONFIG_TOKENS		50
#define MAX_CONFIG_FILES	16

#define NO_CONFIG -1
#define TOO_MANY_CONFIGS -2

/* validate the grasp config option */
#define CONFIG_VALIDATE_OPTN(C, h, n, N)                                    \
	do {                                                                \
		int __c = config_get_item_values(h, n);                     \
		int __d;                                                    \
		if (__c != N) {                                             \
			SHOUT("grasp: %s defined incorrectly "              \
				"or undefined\n", n);                       \
			return -EINVAL;                                     \
		}                                                           \
		for (__d = 0; __d < __c; __d++) {                           \
			C[__d] = config_get_item_value(h, n, __d);          \
			DBG("%s: %s\n", n, C[__d]);                         \
		}                                                           \
	} while (0);

#define CONFIG_VALIDATE_OPTM(C, h, n, N)                                    \
	do {                                                                \
		int __c = config_get_item_values(h, n);                     \
		int __d;                                                    \
		if (__c < N) {                                              \
			SHOUT("grasp: %s defined incorrectly "              \
				"or undefined\n", n);                       \
			return -EINVAL;                                     \
		}                                                           \
		N = __c;                                                    \
		for (__d = 0; __d < __c; __d++) {                           \
			C[__d] = config_get_item_value(h, n, __d);          \
			DBG("%s: %s\n", n, C[__d]);                         \
		}                                                           \
	} while (0);

#define CONFIG_VALIDATE_OPT(C, h, n)                                        \
	do {                                                                \
		char *__C[1];                                               \
		CONFIG_VALIDATE_OPTN(__C, h, n, 1);                         \
		C = __C[0];                                                 \
	} while (0);

typedef struct __config_item {
	US                      name;
	USS                     value;
	int                     values;
	struct __config_item    *next;
	struct __config_item    *prev;
}       config_item_t;

typedef struct __config {
	US              path;
	config_item_t   *first_item;
	config_item_t   *last_item;
}       config_t;

int             config_read(US file);
int             config_write(int n, US filename);
int             config_dispose(int n);
config_item_t   *config_item_search(int n, US name);
int             config_del_item(int n, config_item_t *item);
int             config_get_item_values(int n, US name);
US              config_get_item_value(int n, US name, int no);

/*
 * Global grasp configuration file options are held in this structure
 * These commonly come from ~/.grasp.
 */
typedef struct __global_config {
	char *gitbase_url;    /* url of a root for git repos      */
	char *gitrepos_dir;   /* local path to store git repos in */
	char *tarballs_dir;   /* local path to store tarballs in  */
	char *reget_grasp;    /* do download grasp file anyway    */
}       global_config_t;

extern global_config_t CONFIG;

/*
 * Package's grasp file downloaded from the server
 */
typedef struct __grasp_config {
	char **tarball_url;   /* urls of package's upstream tarballs */
	char **tarball_md5;   /* md5s of package's upstream tarballs */
	char **branch;        /* names of git branches               */
	char *format;         /* grasp config file format version    */

	/* values below are not read from grasp file, but filled in
	 * during prologue stage                                    */
	char pkgname[PKGNAME_MAX];       /* package name            */
	char **tarball_path;             /* paths to local tarballs */
	int  ntarballs;       /* number of tarballs for the package */
	int  nbranches;       /* number of branches for the package */
	int  move_grasp;      /* do move /tmp/grasp to pkg/.git     */
	char gitrepo_path[PATH_MAX];     /* paths to local git repo */
	char graspfile_local[PATH_MAX];     /* local copy of grasp  */
}       grasp_config_t;

extern grasp_config_t GRASP;

#endif /* __CONFIG_H__ */
