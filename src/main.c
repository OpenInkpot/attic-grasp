#include "common.h"
#include <popt.h>

FILE *OUT[OUTFILES_MAX];
int verbosity;

/* cli overrides for global config parameters */
static char *CMDLINE_gitbase_url;
static char *CMDLINE_tarballs_dir;
static char *CMDLINE_gitrepos_dir;

#define CMDLINE_CONFIG(__c, __n) \
	if (CMDLINE_ ## __n) { \
		free(__c); \
		__c = CMDLINE_ ## __n; \
	}

static struct poptOption opts_table[] = {
	/* CONFIG-wise options */
	{ "gitbase-url",  'U', POPT_ARG_STRING, &CMDLINE_gitbase_url, 0,
	  "the base URL for your git repositories" },
	{ "tarballs-dir", 'T', POPT_ARG_STRING, &CMDLINE_tarballs_dir, 0,
	  "local directory that will hold packages' upstream tarballs" },
	{ "gitrepos-dir", 'G', POPT_ARG_STRING, &CMDLINE_gitrepos_dir, 0,
	  "local directory that will hold packages' git repos" },
	{ "reget-grasp",  'a', 0, 0, 'a', "always redownload remote grasp" },

	/* simple options */
	{ "verbose",   'v', 0, 0, 'v', "turn on debugging output" },
	{ "version",   'V', 0, 0, 'V', "show our version number"  },
	{ "help",      'h', 0, 0, 'h', "print help message"       },

	{ NULL,        0,   0, NULL, 0 }
};

static cmdfn command[] = {
	cmd_getpkg,
	cmd_update,
	cmd_build
};

char PWD[PATH_MAX];

static struct sigaction __sigint_act_saved;

static void sigint_handler(int sig)
{
	SAY("Control-C pressed.\n");
	signal(sig, NULL);
	leave(EXIT_FAILURE, 0);
}

void init()
{
	struct sigaction sa;
	int gv;

	/* initialize output */
	verbosity = VERB_NORMAL;
	OUT[STD] = stdout;
	OUT[ERR] = stderr;
	OUT[LOG] = stdout; /* XXX */

	/* save $PWD */
	if (!getcwd(PWD, PATH_MAX)) {
		perror("getcwd");
		exit(EXIT_FAILURE);
	}

	sa.sa_handler = sigint_handler;
	sigfillset(&sa.sa_mask);
	sigaction(SIGINT, &sa, &__sigint_act_saved);

	git_init();
}

void leave(int code, int full)
{
	if (full)
		global_config_done();
	sigaction(SIGINT, &__sigint_act_saved, NULL);
	exit(code);
}

int main(int argc, const char **argv, const char **envp)
{
	int c;
	poptContext optcon;
	int cmd = 0;
	void *cmddata = NULL;
	char *cmdname = NULL;
	char *pkgname = NULL;
	uid_t uid = geteuid();

	init();
	if (uid == 0) {
		SHOUT("Under no circumstances is grasp going to work "
				"with effective uid of root.\n");
		leave(EXIT_FAILURE, 0);
	}

	optcon = poptGetContext(NULL, argc, argv, opts_table, 0);
	poptSetOtherOptionHelp(optcon, "[<option>] <command> [<package> [<branch>]]");
	while ((c = poptGetNextOpt(optcon)) >= 0) {
		switch (c) {
			case 'a':
				CONFIG.reget_grasp = 0;
				break;

			case 'h':
				help();
				poptPrintHelp(optcon, stdout, 0);
				help_cmd();
				leave(EXIT_SUCCESS, 0);

			case 'V':
				version();
				leave(EXIT_SUCCESS, 0);

			case 'v':
				verbosity = VERB_DEBUG;
				break;

			default:
				SHOUT("this option just sucks ass\n");
				leave(EXIT_FAILURE, 0);
		}
	}

	if (c < -1) {
		poptPrintUsage(optcon, stderr, 0);
		poptFreeContext(optcon);
		leave(EXIT_FAILURE, 0);
	}

	/* read global config */
	c = global_config_init();
	if (c) leave(EXIT_FAILURE, 0);

	CMDLINE_CONFIG(CONFIG.gitbase_url, gitbase_url)
	CMDLINE_CONFIG(CONFIG.gitrepos_dir, gitrepos_dir)
	CMDLINE_CONFIG(CONFIG.tarballs_dir, tarballs_dir)

	/* validate command name and arguments */
	cmdname = (US)poptGetArg(optcon);
	if (!cmdname) {
		SAY("A command is required\n");
		leave(EXIT_FAILURE, 1);
	}

	DBG("cmd=%s\n", cmdname);
	if (!strcmp(cmdname, "getpkg")) {
		pkgname = (US)poptGetArg(optcon);
		if (!pkgname) {
			SHOUT("A package name is required\n");
			leave(EXIT_FAILURE, 1);
		}

		cmd = CMD_GETPKG;
		cmddata = NULL;
	} else if (!strcmp(cmdname, "update")) {
		pkgname = (US)poptGetArg(optcon);
		if (!pkgname) {
			SHOUT("A package name is required\n");
			leave(EXIT_FAILURE, 1);
		}

		cmd = CMD_UPDATE;

		/* in case a branch name is given */
		cmddata = (void *)poptGetArg(optcon);
	} else if (!strcmp(cmdname, "build")) {
		pkgname = (US)poptGetArg(optcon);
		if (!pkgname) {
			SHOUT("A package name is required\n");
			leave(EXIT_FAILURE, 1);
		}

		cmd = CMD_BUILD;
		cmddata = NULL;
	} else {
		SHOUT("Error: Unknown command %s, exiting.\n", cmdname);
		leave(EXIT_FAILURE, 0);
	}

	/* execute the command */
	switch (cmd) {
		case CMD_GETPKG:
		case CMD_UPDATE:
		case CMD_BUILD:
			c = pkg_cmd_prologue(pkgname);
			if (c == GE_ERROR)
				leave(EXIT_FAILURE, 1);

			/* we cannot do anything without package's git repo */
			cmd = (c & GS_NOLOCALCOPY) ? CMD_GETPKG : cmd;

			/* go for it */
			c = (command[cmd])(cmddata);
			if (c == GE_ERROR) break;

			/* clean up */
			c = pkg_cmd_epilogue();
			if (c == GE_ERROR)
				leave(EXIT_FAILURE, 1);
			break;

		default:
			SHOUT("WTF?!\n");
			leave(EXIT_FAILURE, 1);
	}

	return c;
}

