SUBDIRS = tests

AM_CXXFLAGS = @JACK_CFLAGS@ @LOSC_CFLAGS@ @ALSA_CFLAGS@ @LASH_CFLAGS@ @SLV2_CFLAGS@ -I$(top_srcdir)/src/common -fno-exceptions -fno-rtti

common_SOURCES = \
	util.h \
	tuning.h \
	Node.h \
	NodeBase.h \
	NodeBase.cpp \
	InternalNode.h \
	Patch.h \
	Patch.cpp \
	NodeFactory.h \
	NodeFactory.cpp \
	Om.h \
	Om.cpp \
	OmApp.h \
	OmApp.cpp \
	JackAudioDriver.h \
	JackAudioDriver.cpp \
	OSCReceiver.h \
	OSCReceiver.cpp \
	Responder.h \
	OSCResponder.h \
	OSCResponder.cpp \
	ClientKey.h \
	ClientBroadcaster.h \
	ClientBroadcaster.cpp \
	ObjectSender.h \
	ObjectSender.cpp \
	OSCClient.h \
	OSCClient.cpp \
	Buffer.h \
	Buffer.cpp \
	Port.h \
	Port.cpp \
	PortBase.h \
	PortBase.cpp \
	InputPort.h \
	InputPort.cpp \
	OutputPort.h \
	OutputPort.cpp \
	MidiMessage.h \
	MidiNoteNode.h \
	MidiNoteNode.cpp \
	MidiTriggerNode.h \
	MidiTriggerNode.cpp \
	MidiControlNode.h \
	MidiControlNode.cpp \
	RegisterClientEvent.h \
	RegisterClientEvent.cpp \
	UnregisterClientEvent.h \
	UnregisterClientEvent.cpp \
	BridgeNode.h \
	BridgeNode.cpp \
	ControlInputNode.h \
	ControlInputNode.cpp \
	ControlOutputNode.h \
	ControlOutputNode.cpp \
	AudioInputNode.h \
	AudioInputNode.cpp \
	AudioOutputNode.h \
	AudioOutputNode.cpp \
	MidiInputNode.h \
	MidiInputNode.cpp \
	MidiOutputNode.h \
	MidiOutputNode.cpp \
	Event.h \
	Event.cpp \
	QueuedEvent.h \
	PingQueuedEvent.h \
	ActivateEvent.h \
	ActivateEvent.cpp \
	DeactivateEvent.h \
	DeactivateEvent.cpp \
	SetPortValueEvent.h \
	SetPortValueEvent.cpp \
	SetPortValueQueuedEvent.h \
	SetPortValueQueuedEvent.cpp \
	NoteOnEvent.h \
	NoteOnEvent.cpp \
	NoteOffEvent.h \
	NoteOffEvent.cpp \
	AllNotesOffEvent.h \
	AllNotesOffEvent.cpp \
	ConnectionEvent.h \
	ConnectionEvent.cpp \
	DisconnectionEvent.h \
	DisconnectionEvent.cpp \
	DisconnectNodeEvent.h \
	DisconnectNodeEvent.cpp \
	DisconnectPortEvent.h \
	DisconnectPortEvent.cpp \
	DestroyEvent.h \
	DestroyEvent.cpp \
	EventSource.h \
	QueuedEventSource.h \
	QueuedEventSource.cpp \
	QueuedEngineInterface.h \
	QueuedEngineInterface.cpp \
	OmObject.h \
	Maid.h \
	Maid.cpp \
	MaidObject.h \
	Tree.h \
	AddNodeEvent.h \
	AddNodeEvent.cpp \
	SetMetadataEvent.h \
	SetMetadataEvent.cpp \
	RequestMetadataEvent.h \
	RequestMetadataEvent.cpp \
	RequestPortValueEvent.h \
	RequestPortValueEvent.cpp \
	RequestAllObjectsEvent.h \
	RequestAllObjectsEvent.cpp \
	RequestPluginsEvent.h \
	RequestPluginsEvent.cpp \
	CreatePatchEvent.h \
	CreatePatchEvent.cpp \
	ClientRecord.h \
	PortInfo.h \
	PluginLibrary.h \
	Plugin.h \
	Array.h \
	List.h \
	PostProcessor.h \
	PostProcessor.cpp \
	Connection.h \
	Connection.cpp \
	ConnectionBase.h \
	ConnectionBase.cpp \
	ObjectStore.h \
	ObjectStore.cpp \
	LoadPluginsEvent.h \
	LoadPluginsEvent.cpp \
	EnablePatchEvent.h \
	EnablePatchEvent.cpp \
	DisablePatchEvent.h \
	DisablePatchEvent.cpp \
	ClearPatchEvent.h \
	ClearPatchEvent.cpp \
	TransportNode.h \
	TransportNode.cpp \
	RenameEvent.h \
	RenameEvent.cpp \
	MidiLearnEvent.h \
	MidiLearnEvent.cpp \
	Driver.h \
	AudioDriver.h \
	MidiDriver.h \
	midi.h \
	../common/util/Semaphore.h \
	../common/util/types.h \
	../common/util/Path.h \
	../common/util/Queue.h \
	../common/interface/ClientInterface.h \
	../common/interface/EngineInterface.h \
	instantiations.cpp

if WITH_JACK_MIDI
common_SOURCES += \
	JackMidiDriver.h \
	JackMidiDriver.cpp
endif

if WITH_ALSA_MIDI
common_SOURCES += \
	AlsaMidiDriver.h \
	AlsaMidiDriver.cpp
endif

if WITH_LADSPA
common_SOURCES += \
	LADSPAPlugin.h \
	LADSPAPlugin.cpp
endif

if WITH_DSSI
common_SOURCES += \
	DSSIPlugin.h \
	DSSIPlugin.cpp \
	DSSIUpdateEvent.h \
	DSSIUpdateEvent.cpp \
	DSSIControlEvent.h \
	DSSIControlEvent.cpp \
	DSSIConfigureEvent.h \
	DSSIConfigureEvent.cpp \
	DSSIProgramEvent.h \
	DSSIProgramEvent.cpp
endif

if WITH_LV2
common_SOURCES += \
	LV2Plugin.h \
	LV2Plugin.cpp
endif

if WITH_LASH
common_SOURCES += LashDriver.h LashDriver.cpp LashRestoreDoneEvent.h
endif


#
# Non-installed library to link stand-alone and in-process build against
#

noinst_LIBRARIES = libom.a
libom_a_SOURCES = $(common_SOURCES)



#
# Stand-alone engine
#
if BUILD_ENGINE

bin_PROGRAMS = om
om_DEPENDENCIES = libom.a
om_LDADD = @JACK_LIBS@ @LOSC_LIBS@ @ALSA_LIBS@ @LASH_LIBS@ @SLV2_LIBS@ -lrt libom.a 

om_SOURCES = \
	main.cpp \
	cmdline.h \
	cmdline.c

endif # BUILD_ENGINE



#
# Jack internal client
#
if BUILD_IN_PROCESS_ENGINE

# FIXME: Figure out how to get this properly
omdir = $(prefix)/lib/jack

om_la_CFLAGS = -fPIC
om_LTLIBRARIES = om.la
om_la_DEPENDENCIES = libom.a
om_la_LIBADD = libom.a
om_la_LDFLAGS = -module -avoid-version @JACK_LIBS@ @LOSC_LIBS@ @ALSA_LIBS@ @LASH_LIBS@ @SLV2_LIBS@
om_la_SOURCES = OmInProcess.cpp

endif # BUILD_IN_PROCESS_ENGINE


