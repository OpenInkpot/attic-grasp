#include "common.h"

void version()
{
	SAY(
		"%s %s for %s compiled on %s\n",
		PACKAGE_NAME, VERSION, HOST_OS, BUILD_DATE
	   );
}

void help()
{
	SAY(
		"grasp is a tool aimed at automated downloading and building\n"
		".deb packages which are being kept under git version control\n"
		"(e.g. http://git.slind.org/).\n\n"
	   );
}

void help_cmd()
{
	SAY(
		"\nCommands:\n"
		"    getpkg - Clone package's git repository and download\n"
		"             all the relevant source tarballs\n"
		"    update - Update local git repository and (re-)download\n"
		"             all the relevant source tarballs if necessary;\n"
		"             this command may take one branch name specified\n"
		"             after package's name, by default does all branches.\n"
		"    build  - Build source packages out of a git repository;\n"
		"             may take one branch name like 'update', otherwise\n"
		"             build all the branches.\n\n"
	   );
}

