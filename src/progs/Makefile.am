AM_CXXFLAGS = -I$(top_srcdir)/src/common

DIST_SUBDIRS = python supercollider

SUBDIRS = om

if BUILD_CONSOLE_CLIENTS
SUBDIRS += patch_loader demolition
endif

if BUILD_GTK_CLIENT
SUBDIRS += gtk
endif


