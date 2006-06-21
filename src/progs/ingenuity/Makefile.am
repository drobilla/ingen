if BUILD_GTK_CLIENT

EXTRA_DIST = ingenuity.gladep
MAINTAINERCLEANFILES = Makefile.in

sharefilesdir = $(pkgdatadir)
dist_sharefiles_DATA = ingenuity.glade om-icon.png

AM_CXXFLAGS = -DGTK_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -I$(top_srcdir)/src/common -I$(top_srcdir)/src/libs/client -DPKGDATADIR=\"$(pkgdatadir)\" @GTKMM_CFLAGS@ @LIBGLADEMM_CFLAGS@ @GNOMECANVASMM_CFLAGS@ @LOSC_CFLAGS@ @LASH_CFLAGS@ @FLOWCANVAS_CFLAGS@ -DWITH_RTTI
ingenuity_LDADD = @GTKMM_LIBS@ @LIBGLADEMM_LIBS@ @GNOMECANVASMM_LIBS@ @LOSC_LIBS@ @LASH_LIBS@ @FLOWCANVAS_LIBS@ ../../libs/client/libomclient.la
ingenuity_DEPENDENCIES = ../../libs/client/libomclient.la


bin_PROGRAMS = ingenuity
ingenuity_SOURCES = \
	cmdline.h \
	cmdline.c \
	main.cpp \
	singletons.cpp \
	ConnectWindow.h \
	ConnectWindow.cpp \
	App.h \
	App.cpp \
	Configuration.h \
	Configuration.cpp \
	GladeFactory.h \
	GladeFactory.cpp \
	Controller.h \
	Controller.cpp \
	GtkObjectController.h \
	GtkObjectController.cpp \
	PatchController.h \
	PatchController.cpp \
	NodeController.h \
	NodeController.cpp \
	PortController.h \
	PortController.cpp \
	DSSIController.h \
	DSSIController.cpp \
	LoadPluginWindow.h \
	LoadPluginWindow.cpp \
	LoadPatchWindow.h \
	LoadPatchWindow.cpp \
	MessagesWindow.h \
	MessagesWindow.cpp \
	LoadSubpatchWindow.h \
	LoadSubpatchWindow.cpp \
	NodeControlWindow.h \
	NodeControlWindow.cpp \
	ControlPanel.h \
	ControlPanel.cpp \
	ControlGroups.h \
	ControlGroups.cpp \
	PatchView.h \
	PatchView.cpp \
	PatchWindow.h \
	PatchWindow.cpp \
	BreadCrumb.h \
	OmFlowCanvas.h \
	OmFlowCanvas.cpp \
	../../common/types.h \
	../../common/Path.h \
	OmModule.h \
	OmModule.cpp \
	OmPortModule.h \
	OmPortModule.cpp \
	DSSIModule.h \
	DSSIModule.cpp \
	SubpatchModule.h \
	SubpatchModule.cpp \
	OmPort.h \
	OmPort.cpp \
	OmPatchPort.h \
	OmPatchPort.cpp \
	NewSubpatchWindow.h \
	NewSubpatchWindow.cpp \
	ConfigWindow.h \
	ConfigWindow.cpp \
	PatchPropertiesWindow.h \
	PatchPropertiesWindow.cpp \
	Loader.h \
	Loader.cpp \
	RenameWindow.h \
	RenameWindow.cpp \
	NodePropertiesWindow.h \
	NodePropertiesWindow.cpp \
	PatchTreeWindow.h \
	PatchTreeWindow.cpp

if WITH_LASH
ingenuity_SOURCES += LashController.h LashController.cpp
endif


endif # WITH_GTK_CLIENT
