AM_CXXFLAGS = -I$(top_srcdir)/src/common -fno-exceptions -fno-rtti

if BUILD_CLIENT_LIB
noinst_LTLIBRARIES = libomclient.la

libomclient_la_CXXFLAGS = -I$(top_srcdir)/src/common -DPKGDATADIR=\"$(pkgdatadir)\" $(LXML2_CFLAGS) $(LSIGCPP_CFLAGS)

libomclient_la_SOURCES = \
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
	Store.h \
	Store.cpp \
	$(top_srcdir)/src/common/util/Path.h \
	$(top_srcdir)/src/common/interface/ClientInterface.h \
	$(top_srcdir)/src/common/interface/EngineInterface.h

endif # BUILD_CLIENT_LIB

