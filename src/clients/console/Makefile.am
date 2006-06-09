om_console_client_CXXFLAGS = -I$(top_srcdir)/src/clients -I$(top_srcdir)/src/common -DPKGDATADIR=\"$(pkgdatadir)\" $(LXML2_CFLAGS) $(LOSC_CFLAGS)
om_console_client_LDADD = ../libomclient.a $(LOSC_LIBS) $(LXML2_LIBS)

bin_PROGRAMS = console_client
EXTRA_DIST = README

console_client_CXXFLAGS = "-I../../common"
console_client_LDADD = @LOSC_LIBS@ ../Comm.o
console_client_SOURCES = \
	console_client.cpp \
	ConsoleClientHooks.h \
	ConsoleClientHooks.cpp
