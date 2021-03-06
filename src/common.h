#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include "config.h"

#ifndef HOST_OS
#define HOST_OS "linux"
#endif
#ifndef BUILD_DATE
#define BUILD_DATE "N/A"
#endif

#define URL_MAX 128
#define PKGNAME_MAX 32
#define MD5_MAX 33 /* add nul character */
#define REFSPEC_MAX 128

#include "types.h"
#include "conffile.h"
#include "output.h"
#include "fileops.h"

extern char PWD[FILENAME_MAX];

/* message.c */
void help();
void help_cmd();
void version();

/* config.c */
int global_config_init();
void global_config_done();

/* graspfile.c */
int get_grasp();
void free_grasp();
int put_grasp(char *filename);

/* getpkg.c */
int cmd_getpkg(void *data);
int cmd_update(void *data);
int cmd_build(void *data);

/* get.c */
int http_get_file(char *url, char *name);

/* git.c */
int mv(char *oldpath, char *newpath);
int md5sum(char *file, char *buf);
int git_clone(char *url);
int git_pull(char *url, char *branch);
char* git_convert_url(const char *url);

/* package.c */
int pkg_cmd_prologue(char *pkgname);
int pkg_cmd_epilogue();

#define CMD_GETPKG 0x00
#define CMD_UPDATE 0x01
#define CMD_BUILD  0x02

#endif
