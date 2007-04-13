#include "common.h"
#include <popt.h>

FILE *OUT[OUTFILES_MAX];
int verbosity;

static struct poptOption opts_table[] = {
	{ "help",      'h', 0, 0, 'h', "print help message"       },
	{ "version",   'V', 0, 0, 'V', "show our version number"  },
	{ "verbose",   'v', 0, 0, 'v', "turn on debugging output" },
	{ NULL,        0,   0, NULL, 0 }
};

static cmdfn command[] = {
	cmd_getpkg,
	cmd_update,
	cmd_build
};

void init()
{
	verbosity = VERB_NORMAL;
	OUT[STD] = stdout;
	OUT[ERR] = stderr;
	OUT[LOG] = stdout; /* XXX */

	/* save $PWD */
	if (!getcwd(PWD, FILENAME_MAX)) {
		perror("getcwd");
		exit(EXIT_FAILURE);
	}
}

void leave(int code)
{
	global_config_done();
	exit(code);
}

char PWD[FILENAME_MAX];

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
		exit(EXIT_FAILURE);
	}

	optcon = poptGetContext(NULL, argc, argv, opts_table, 0);
	while ((c = poptGetNextOpt(optcon)) >= 0) {
		switch (c) {
			case 'h':
				help();
				exit(EXIT_SUCCESS);

			case 'V':
				version();
				exit(EXIT_SUCCESS);

			case 'v':
				verbosity = VERB_DEBUG;
				break;

			default:
				SHOUT("this option just sucks ass\n");
				exit(EXIT_FAILURE);
		}
	}

	if (c < -1) {
		help();
		exit(EXIT_FAILURE);
	}

	/* read global config */
	c = global_config_init();
	if (c) exit(EXIT_FAILURE);

	/* validate command name and arguments */
	cmdname = (US)poptGetArg(optcon);
	if (!cmdname) {
		SAY("A command is required\n");
		leave(EXIT_FAILURE);
	}

	DBG("cmd=%s\n", cmdname);
	if (!strcmp(cmdname, "getpkg")) {
		pkgname = (US)poptGetArg(optcon);
		if (!pkgname) {
			SHOUT("A package name is required\n");
			leave(EXIT_FAILURE);
		}

		cmd = CMD_GETPKG;
		cmddata = NULL;
	} else if (!strcmp(cmdname, "update")) {
		pkgname = (US)poptGetArg(optcon);
		if (!pkgname) {
			SHOUT("A package name is required\n");
			leave(EXIT_FAILURE);
		}

		cmd = CMD_UPDATE;

		/* in case a branch name is given */
		cmddata = (void *)poptGetArg(optcon);
	} else if (!strcmp(cmdname, "build")) {
		pkgname = (US)poptGetArg(optcon);
		if (!pkgname) {
			SHOUT("A package name is required\n");
			leave(EXIT_FAILURE);
		}

		cmd = CMD_BUILD;
		cmddata = NULL;
	} else {
		SHOUT("Error: Unknown command %s, exiting.\n", cmdname);
		leave(EXIT_FAILURE);
	}

	/* execute the command */
	switch (cmd) {
		case CMD_GETPKG:
		case CMD_UPDATE:
		case CMD_BUILD:
			c = pkg_cmd_prologue(pkgname);
			if (c == GE_ERROR)
				leave(EXIT_FAILURE);

			/* we cannot do anything without package's git repo */
			cmd = (c & GS_NOLOCALCOPY) ? CMD_GETPKG : cmd;

			/* go for it */
			c = (command[cmd])(cmddata);
			if (c == GE_ERROR) break;

			/* clean up */
			c = pkg_cmd_epilogue();
			if (c == GE_ERROR)
				leave(EXIT_FAILURE);
			break;

		default:
			SHOUT("WTF?!\n");
			leave(EXIT_FAILURE);
	}

	return c;
}

