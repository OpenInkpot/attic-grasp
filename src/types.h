#ifndef __TYPES_H__
#define __TYPES_H__

/* grasp status */
#define GS_OK           0x00 /* everything's fine */
#define GS_NOLOCALCOPY  0x01 /* directory for local copy isn't there */
#define GS_NOGITDIR     0x02 /* no directory for git repos */
#define GS_NOTARBALLDIR 0x04 /* no directory for upstream tarballs */

/* grasp errors */
#define GE_OK             0
#define GE_ERROR        (-1) /* general error */
#define GE_ACCESS       (-2) 
#define GE_NOENT        (-3) 

typedef int (*cmdfn)(void *);
typedef unsigned int  UI;
typedef unsigned long UL;
typedef /*unsigned */char UC;
typedef UC*           US;
typedef US*           USS;
typedef char          CU;
typedef CU*           SU;
typedef SU*           SSU;
typedef void*         VP;

#define ustrlen(__s) strlen((SU)__s)
#define ustrcmp(__a,__b) strcmp((SU)__a,(SU)__b)
#define ustrncmp(__a,__b,__n) strcmp((SU)__a,(SU)__b,__n)
#define ustrcpy(__a,__b) strcpy((SU)__a,(SU)__b)
#define ustrncpy(__a,__b,__n) strncpy((SU)__a,(SU)__b,__n)
#define uatoi(__a) atoi((SU)__a)

#define xmalloc(__s) ({                    \
		void *__ret = malloc(__s); \
		if (!__ret)                \
			return GE_ERROR;   \
		__ret;                     \
	})

#define xfree(__p) do {                      \
		void **__P = (void *)&(__p); \
		if (*__P)                    \
			free(*__P);          \
		*__P = NULL;                 \
	} while (0)

#endif
