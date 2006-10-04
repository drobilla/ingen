AM_CXXFLAGS = -I$(top_srcdir)/src/common

DIST_SUBDIRS = python supercollider patch_loader ingenuity server demolition

SUBDIRS = server

if BUILD_CONSOLE_CLIENTS
SUBDIRS += patch_loader # demolition
endif

if BUILD_GTK_CLIENT
SUBDIRS += ingenuity
endif


