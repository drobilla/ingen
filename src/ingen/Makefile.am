AM_CXXFLAGS = \
	-DINGEN_MODULE_DIR=\"$(libdir)/ingen\" \
	@GTHREAD_CFLAGS@ \
	@GLIBMM_CFLAGS@ \
	@REDLANDMM_CFLAGS@ \
	@RAUL_CFLAGS@ \
	@SLV2_CFLAGS@ \
	@INGEN_CFLAGS@

MAINTAINERCLEANFILES = Makefile.in

desktopfilesdir = $(datadir)/applications
dist_desktopfiles_DATA = ingen.desktop

bin_PROGRAMS = ingen
ingen_LDADD = \
	../module/libingen_module.la \
	../shared/libingen_shared.la \
	@SLV2_LIBS@ \
	@REDLANDMM_LIBS@ \
	@RAUL_LIBS@ \
	@GTHREAD_LIBS@ \
	@GLIBMM_LIBS@

ingen_DEPENDENCIES = ../module/libingen_module.la

ingen_SOURCES = \
	main.cpp \
	cmdline.h \
	cmdline.c
