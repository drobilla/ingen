AM_CXXFLAGS = @JACK_CFLAGS@ @LOSC_CFLAGS@ @ALSA_CFLAGS@ @LASH_CFLAGS@ @SLV2_CFLAGS@ -I$(top_srcdir)/src/common -I$(top_srcdir)/src/engine -I$(top_srcdir)/src/engine/events -fno-exceptions -fno-rtti

MAINTAINERCLEANFILES = Makefile.in

#
# Stand-alone engine
#
if BUILD_ENGINE

bin_PROGRAMS = om
om_DEPENDENCIES = ../../libs/engine/libom.la
om_LDADD = @JACK_LIBS@ @LOSC_LIBS@ @ALSA_LIBS@ @LASH_LIBS@ @SLV2_LIBS@ -lrt ../../libs/engine/libom.la

om_SOURCES = \
	main.cpp \
	cmdline.h \
	cmdline.c

endif # BUILD_ENGINE


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




