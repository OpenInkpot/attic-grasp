bin_PROGRAMS = grasp

grasp_SOURCES = main.c transport.c commands.c conffile.c graspfile.c \
		system.c package.c messages.c pool.c git.c
grasp_CFLAGS = -DBUILD_DATE="\"$(BUILD_DATE)\""
grasp_LDADD = $(CURL_LIBS) $(POPT_LIBS)

