AM_CXXFLAGS = @RAUL_CFLAGS@ @SLV2_CFLAGS@ @JACK_CFLAGS@ @LOSC_CFLAGS@ @ALSA_CFLAGS@ @LASH_CFLAGS@ -I$(top_srcdir)/src/common -I$(top_srcdir)/src/libs/engine -I$(top_srcdir)/src/libs/engine/events

MAINTAINERCLEANFILES = Makefile.in

#
# Stand-alone engine
#
if BUILD_SERVER

bin_PROGRAMS = ingen
ingen_DEPENDENCIES = ../../libs/engine/libingen.la
ingen_LDADD = @RAUL_LIBS@ @SLV2_LIBS@ @JACK_LIBS@ @LOSC_LIBS@ @ALSA_LIBS@ @LASH_LIBS@ -lrt ../../libs/engine/libingen.la

ingen_SOURCES = \
	main.cpp \
	cmdline.h \
	cmdline.c

endif # BUILD_SERVER


##
## Jack internal client
##
#if BUILD_IN_PROCESS_ENGINE
#
#
## FIXME: broken
#
#
## FIXME: Figure out how to get this properly
#omdir = $(prefix)/lib/jack
#
#om_la_CFLAGS = -fPIC
#om_LTLIBRARIES = om.la
#om_la_LDFLAGS = -module -avoid-version @JACK_LIBS@ @LOSC_LIBS@ @ALSA_LIBS@ @LASH_LIBS@ @SLV2_LIBS@
#om_la_SOURCES = OmInProcess.cpp
#
#endif # BUILD_IN_PROCESS_ENGINE




