AM_CXXFLAGS = -I$(top_srcdir)/src/common -fno-exceptions -fno-rtti

SUBDIRS = . python supercollider

if BUILD_CONSOLE_CLIENTS
noinst_LIBRARIES = libomclient.a

libomclient_a_CXXFLAGS = -I$(top_srcdir)/src/common -DPKGDATADIR=\"$(pkgdatadir)\" $(LIBGLADEMM_CFLAGS) $(GNOMECANVASMM_CFLAGS) $(JACK_CFLAGS) $(LXML2_CFLAGS)

libomclient_a_SOURCES = \
	ClientInterface.h \
	OSCEngineInterface.h \
	OSCEngineInterface.cpp \
	OSCModelEngineInterface.h \
	OSCModelEngineInterface.cpp \
	OSCListener.h \
	OSCListener.cpp \
	SigClientInterface.h \
	DirectSigClientInterface.h \
	ThreadedSigClientInterface.h \
	ThreadedSigClientInterface.cpp \
	ModelEngineInterface.h \
	ModelClientInterface.h \
	ModelClientInterface.cpp \
	PresetModel.h \
	ControlModel.h \
	ObjectController.h \
	ObjectModel.h \
	ObjectModel.cpp \
	NodeModel.h \
	NodeModel.cpp \
	PortModel.h \
	PatchModel.h \
	PatchModel.cpp \
	PluginModel.h \
	PatchLibrarian.h \
	PatchLibrarian.cpp \
	ConnectionModel.h \
	ConnectionModel.cpp \
	$(top_srcdir)/src/common/util/Path.h \
	$(top_srcdir)/src/common/interface/ClientInterface.h \
	$(top_srcdir)/src/common/interface/EngineInterface.h

SUBDIRS += patch_loader patches demolition

endif # BUILD_CONSOLE_CLIENTS

if BUILD_GTK_CLIENT
SUBDIRS += gtk
endif

