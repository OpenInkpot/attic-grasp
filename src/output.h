#include <stdarg.h>
#include "common.h"

#define OUTFILES_MAX 3

extern FILE *OUT[OUTFILES_MAX];
extern int verbosity;

#define STD 0
#define ERR 1
#define LOG 2

#define VERB_SILENT 0
#define VERB_NORMAL 2
#define VERB_DEBUG  4

static void output(int w, int v, const char *fmt, ...)
{
	va_list ap;

	if (v > verbosity) return;

	va_start(ap, fmt);
	vfprintf(OUT[w], fmt, ap);
	va_end(ap);
}

#define SAY(fmt, args...)                      \
	output(STD, VERB_NORMAL, fmt, ## args)
#define DBG(fmt, args...)                      \
	output(ERR, VERB_DEBUG, "[DBG] " fmt, ## args)
#define SHOUT(fmt, args...)                    \
	output(ERR, VERB_SILENT, "[!!!] " fmt, ## args)

