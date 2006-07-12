SUBDIRS = tests
DIST_SUBDIRS = events

AM_CXXFLAGS = @JACK_CFLAGS@ @LOSC_CFLAGS@ @ALSA_CFLAGS@ @LASH_CFLAGS@ @SLV2_CFLAGS@ -I$(top_srcdir)/src/common -I$(top_srcdir)/src/libs/engine/events -fno-exceptions

MAINTAINERCLEANFILES = Makefile.in

noinst_LTLIBRARIES = libingen.la

libingen_la_SOURCES = \
	util.h \
	tuning.h \
	DataType.h \
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
	TypedPort.h \
	TypedPort.cpp \
	InputPort.h \
	InputPort.cpp \
	OutputPort.h \
	DuplexPort.h \
	DuplexPort.cpp \
	MidiMessage.h \
	MidiNoteNode.h \
	MidiNoteNode.cpp \
	MidiTriggerNode.h \
	MidiTriggerNode.cpp \
	MidiControlNode.h \
	MidiControlNode.cpp \
	Event.h \
	Event.cpp \
	QueuedEvent.h \
	EventSource.h \
	QueuedEventSource.h \
	QueuedEventSource.cpp \
	QueuedEngineInterface.h \
	QueuedEngineInterface.cpp \
	GraphObject.h \
	Maid.h \
	Maid.cpp \
	MaidObject.h \
	Tree.h \
	ClientRecord.h \
	PluginLibrary.h \
	Plugin.h \
	Array.h \
	List.h \
	Slave.h \
	Thread.h \
	Thread.cpp \
	PostProcessor.h \
	PostProcessor.cpp \
	Connection.h \
	Connection.cpp \
	TypedConnection.h \
	TypedConnection.cpp \
	ObjectStore.h \
	ObjectStore.cpp \
	TransportNode.h \
	TransportNode.cpp \
	Driver.h \
	AudioDriver.h \
	MidiDriver.h \
	midi.h \
	../libingen_la/util/Semaphore.h \
	../libingen_la/util/types.h \
	../libingen_la/util/Path.h \
	../libingen_la/util/Queue.h \
	../libingen_la/interface/ClientInterface.h \
	../libingen_la/interface/EngineInterface.h \
	instantiations.cpp \
	events/RegisterClientEvent.h \
	events/RegisterClientEvent.cpp \
	events/UnregisterClientEvent.h \
	events/UnregisterClientEvent.cpp \
	events/PingQueuedEvent.h \
	events/ActivateEvent.h \
	events/ActivateEvent.cpp \
	events/DeactivateEvent.h \
	events/DeactivateEvent.cpp \
	events/SetPortValueEvent.h \
	events/SetPortValueEvent.cpp \
	events/SetPortValueQueuedEvent.h \
	events/SetPortValueQueuedEvent.cpp \
	events/NoteOnEvent.h \
	events/NoteOnEvent.cpp \
	events/NoteOffEvent.h \
	events/NoteOffEvent.cpp \
	events/AllNotesOffEvent.h \
	events/AllNotesOffEvent.cpp \
	events/ConnectionEvent.h \
	events/ConnectionEvent.cpp \
	events/DisconnectionEvent.h \
	events/DisconnectionEvent.cpp \
	events/DisconnectNodeEvent.h \
	events/DisconnectNodeEvent.cpp \
	events/DisconnectPortEvent.h \
	events/DisconnectPortEvent.cpp \
	events/DestroyEvent.h \
	events/DestroyEvent.cpp \
	events/AddPortEvent.h \
	events/AddPortEvent.cpp \
	events/AddNodeEvent.h \
	events/AddNodeEvent.cpp \
	events/SetMetadataEvent.h \
	events/SetMetadataEvent.cpp \
	events/RequestMetadataEvent.h \
	events/RequestMetadataEvent.cpp \
	events/RequestPortValueEvent.h \
	events/RequestPortValueEvent.cpp \
	events/RequestAllObjectsEvent.h \
	events/RequestAllObjectsEvent.cpp \
	events/RequestPluginsEvent.h \
	events/RequestPluginsEvent.cpp \
	events/CreatePatchEvent.h \
	events/CreatePatchEvent.cpp \
	events/LoadPluginsEvent.h \
	events/LoadPluginsEvent.cpp \
	events/EnablePatchEvent.h \
	events/EnablePatchEvent.cpp \
	events/DisablePatchEvent.h \
	events/DisablePatchEvent.cpp \
	events/ClearPatchEvent.h \
	events/ClearPatchEvent.cpp \
	events/RenameEvent.h \
	events/RenameEvent.cpp \
	events/MidiLearnEvent.h \
	events/MidiLearnEvent.cpp \
	types.h
# 	BridgeNode.h 
# 	BridgeNode.cpp 
# 	ControlInputNode.h 
# 	ControlInputNode.cpp 
# 	ControlOutputNode.h 
# 	ControlOutputNode.cpp 
# 	AudioInputNode.h 
# 	AudioInputNode.cpp 
# 	AudioOutputNode.h 
# 	AudioOutputNode.cpp 
# 	MidiInputNode.h 
# 	MidiInputNode.cpp 
# 	MidiOutputNode.h 
# 	MidiOutputNode.cpp 

if WITH_JACK_MIDI
libingen_la_SOURCES += \
	JackMidiDriver.h \
	JackMidiDriver.cpp
endif

if WITH_ALSA_MIDI
libingen_la_SOURCES += \
	AlsaMidiDriver.h \
	AlsaMidiDriver.cpp
endif

if WITH_LADSPA
libingen_la_SOURCES += \
	LADSPANode.h \
	LADSPANode.cpp
endif

if WITH_DSSI
libingen_la_SOURCES += \
	DSSINode.h \
	DSSINode.cpp \
	events/DSSIConfigureEvent.cpp \
	events/DSSIConfigureEvent.h \
	events/DSSIControlEvent.cpp \
	events/DSSIControlEvent.h \
	events/DSSIProgramEvent.cpp \
	events/DSSIProgramEvent.h \
	events/DSSIUpdateEvent.cpp \
	events/DSSIUpdateEvent.h
endif

if WITH_LV2
libingen_la_SOURCES += \
	LV2Node.h \
	LV2Node.cpp
endif

if WITH_LASH
libingen_la_SOURCES += \
	LashDriver.h \
	LashDriver.cpp \
	LashRestoreDoneEvent.h
endif

