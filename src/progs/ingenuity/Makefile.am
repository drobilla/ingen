if BUILD_GTK_CLIENT

EXTRA_DIST = ingenuity.gladep ingenuity_dev
MAINTAINERCLEANFILES = Makefile.in

sharefilesdir = $(pkgdatadir)
dist_sharefiles_DATA = ingenuity.glade ingen.svg

desktopfilesdir = $(datadir)/applications
dist_desktopfiles_DATA = ingenuity.desktop

globalpixmapsdir = $(datadir)/pixmaps
dist_globalpixmaps_DATA = ingen.svg

ingenuity_CXXFLAGS = -DGTK_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -I$(top_srcdir)/src/common -I$(top_srcdir)/src/libs -DPKGDATADIR=\"$(pkgdatadir)\" @RAUL_CFLAGS@ @GTKMM_CFLAGS@ @LIBGLADEMM_CFLAGS@ @GNOMECANVASMM_CFLAGS@ @LOSC_CFLAGS@ @LASH_CFLAGS@ @FLOWCANVAS_CFLAGS@ @SLV2_CFLAGS@ @CURL_CFLAGS@ -DINGEN_MODULE_DIR=\"$(libdir)/ingen\"
ingenuity_LDADD = @RAUL_LIBS@ @GTKMM_LIBS@ @LIBGLADEMM_LIBS@ @GNOMECANVASMM_LIBS@ @LOSC_LIBS@ @LASH_LIBS@ @FLOWCANVAS_LIBS@ @SLV2_LIBS@ @CURL_LIBS@ ../../libs/client/libingen_client.la ../../libs/module/libingen_module.la
ingenuity_DEPENDENCIES = ../../libs/client/libingen_client.la ../../libs/module/libingen_module.la

bin_PROGRAMS = ingenuity
ingenuity_SOURCES = \
	cmdline.h \
	cmdline.c \
	main.cpp \
	NodeMenu.h \
	NodeMenu.cpp \
	PatchCanvas.h \
	PatchCanvas.cpp \
	BreadCrumb.h \
	BreadCrumbBox.h \
	BreadCrumbBox.cpp \
	ConnectWindow.h \
	ConnectWindow.cpp \
	App.h \
	App.cpp \
	Configuration.h \
	Configuration.cpp \
	GladeFactory.h \
	GladeFactory.cpp \
	LoadPluginWindow.h \
	LoadPluginWindow.cpp \
	LoadPatchWindow.h \
	LoadPatchWindow.cpp \
	LoadRemotePatchWindow.h \
	LoadRemotePatchWindow.cpp \
	UploadPatchWindow.h \
	UploadPatchWindow.cpp \
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
	WindowFactory.h \
	WindowFactory.cpp \
	NodeModule.h \
	NodeModule.cpp \
	PatchPortModule.h \
	PatchPortModule.cpp \
	DSSIModule.h \
	DSSIModule.cpp \
	DSSIController.h \
	DSSIController.cpp \
	SubpatchModule.h \
	SubpatchModule.cpp \
	Port.h \
	Port.cpp \
	Connection.h \
	NewSubpatchWindow.h \
	NewSubpatchWindow.cpp \
	ConfigWindow.h \
	ConfigWindow.cpp \
	PatchPropertiesWindow.h \
	PatchPropertiesWindow.cpp \
	ThreadedLoader.h \
	ThreadedLoader.cpp \
	RenameWindow.h \
	RenameWindow.cpp \
	NodePropertiesWindow.h \
	NodePropertiesWindow.cpp \
	PortPropertiesWindow.h \
	PortPropertiesWindow.cpp \
	PatchTreeWindow.h \
	PatchTreeWindow.cpp

if WITH_LASH
ingenuity_SOURCES += LashController.h LashController.cpp
endif


endif # WITH_GTK_CLIENT
