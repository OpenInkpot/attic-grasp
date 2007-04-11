#include "common.h"
#include <getopt.h>
#include <curl/curl.h>

FILE *OUT[OUTFILES_MAX];
int verbosity;

static struct option long_options[] = {
	{ "help",      0, NULL, 'h'  },
	{ "version",   0, NULL, 'V'  },
	{ "verbose",   0, NULL, 'v'  },
	{ NULL,        0, NULL, '\0' },
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
}

char PWD[FILENAME_MAX];

int main(int argc, char **argv, char **envp)
{
	int c, optidx = 1;
	int cmd = 0;
	void *cmddata = NULL;
	char *pkgname = NULL;
	uid_t uid = geteuid();

	init();
	if (uid == 0) {
		SHOUT("Under no circumstances is grasp going to work "
				"with effective uid of root.\n");
		exit(EXIT_FAILURE);
	}

	while (1) {
		c = getopt_long(argc, argv, "hvV", long_options, &optidx);
		if (c == -1) break;

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

	/* save $PWD */
	if (!getcwd(PWD, FILENAME_MAX)) {
		perror("getcwd");
		exit(EXIT_FAILURE);
	}

	/* read global config */
	c = global_config_init();
	if (c) exit(EXIT_FAILURE);

	/* validate command name and arguments */
	if ((argc - optidx) < 1) {
		SAY("A command is required\n");
		exit(EXIT_FAILURE);
	}

	DBG("cmd=%s\n", argv[optidx]);
	if (!strcmp(argv[optidx], "getpkg")) {
		if ((argc - optidx) < 2) {
			SHOUT("A package name is required\n");
			exit(EXIT_FAILURE);
		}

		cmd = CMD_GETPKG;
		cmddata = NULL;
		pkgname = argv[++optidx];
	} else if (!strcmp(argv[optidx], "update")) {
		if ((argc - optidx) < 2) {
			SHOUT("A package name is required\n");
			exit(EXIT_FAILURE);
		}

		cmd = CMD_UPDATE;
		cmddata = NULL;
		pkgname = argv[++optidx];

		/* branch name is given */
		if ((argc - optidx) == 2)
			cmddata = (void *)argv[++optidx];
	} else if (!strcmp(argv[optidx], "build")) {
		if ((argc - optidx) < 2) {
			SHOUT("A package name is required\n");
			exit(EXIT_FAILURE);
		}

		cmd = CMD_BUILD;
		cmddata = NULL;
		pkgname = argv[++optidx];
	} else {
		SHOUT("Error: Unknown command %s, exiting.\n", argv[optidx]);
		exit(EXIT_FAILURE);
	}

	/* execute the command */
	switch (cmd) {
		case CMD_GETPKG:
		case CMD_UPDATE:
		case CMD_BUILD:
			c = pkg_cmd_prologue(pkgname);
			if (c == GE_ERROR)
				exit(EXIT_FAILURE);

			/* we cannot do anything without package's git repo */
			cmd = (c & GS_NOLOCALCOPY) ? CMD_GETPKG : cmd;

			/* go for it */
			c = (command[cmd])(cmddata);
			if (c == GE_ERROR) break;

			/* clean up */
			c = pkg_cmd_epilogue();
			if (c == GE_ERROR)
				exit(EXIT_FAILURE);
			break;

		default:
			SHOUT("WTF?!\n");
			exit(EXIT_FAILURE);
	}

	return c;
}

