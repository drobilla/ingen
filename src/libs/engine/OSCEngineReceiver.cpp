/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
 * 
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "OSCEngineReceiver.h"
#include <iostream>
#include <cstdlib>
#include <string>
#include <lo/lo.h>
#include "types.h"
#include "util/Queue.h"
#include "util/CountedPtr.h"
#include "QueuedEventSource.h"
#include "interface/ClientKey.h"
#include "interface/ClientInterface.h"
#include "OSCClientSender.h"
#include "OSCResponder.h"
#include "ClientBroadcaster.h"

using std::cerr; using std::cout; using std::endl;

namespace Ingen {

using Shared::ClientKey;


/*! \page engine_osc_namespace Engine OSC Namespace Documentation
 *
 * <p>These are the commands the engine recognizes.  A client can control every
 * aspect of the engine entirely with these commands.</p>
 *
 * <p>All commands on this page are in the "control band".  If a client needs to
 * know about the state of the engine, it must listen to the "notification band".
 * See the "Client OSC Namespace Documentation" for details.
 */


OSCEngineReceiver::OSCEngineReceiver(Engine& engine, size_t queue_size, const char* const port)
: QueuedEngineInterface(engine, queue_size, queue_size), // FIXME
  _port(port),
  _server(NULL),
  _osc_responder(NULL)
{
	_server = lo_server_new(port, error_cb);
	
	if (_server == NULL) {
		cerr << "[OSC] Could not start OSC server.  Aborting." << endl;
		exit(EXIT_FAILURE);
	} else {
		char* lo_url = lo_server_get_url(_server);
		cout << "[OSC] Started OSC server at " << lo_url << endl;
		free(lo_url);
	}

	// For debugging, print all incoming OSC messages
	lo_server_add_method(_server, NULL, NULL, generic_cb, NULL);

	// Set response address for this message.
	// It's important this is first and returns nonzero.
	lo_server_add_method(_server, NULL, NULL, set_response_address_cb, this);
	
	// Commands
	lo_server_add_method(_server, "/om/ping", "i", ping_cb, this);
	lo_server_add_method(_server, "/om/ping_slow", "i", ping_slow_cb, this);
	lo_server_add_method(_server, "/om/engine/quit", "i", quit_cb, this);
	//lo_server_add_method(_server, "/om/engine/register_client", "is", register_client_cb, this);
	lo_server_add_method(_server, "/om/engine/register_client", "i", register_client_cb, this);
	lo_server_add_method(_server, "/om/engine/unregister_client", "i", unregister_client_cb, this);
	lo_server_add_method(_server, "/om/engine/load_plugins", "i", load_plugins_cb, this);
	lo_server_add_method(_server, "/om/engine/activate", "i", engine_activate_cb, this);
	lo_server_add_method(_server, "/om/engine/deactivate", "i", engine_deactivate_cb, this);
	lo_server_add_method(_server, "/om/synth/create_patch", "isi", create_patch_cb, this);
	lo_server_add_method(_server, "/om/synth/enable_patch", "is", enable_patch_cb, this);
	lo_server_add_method(_server, "/om/synth/disable_patch", "is", disable_patch_cb, this);
	lo_server_add_method(_server, "/om/synth/clear_patch", "is", clear_patch_cb, this);
	lo_server_add_method(_server, "/om/synth/create_port", "issi", create_port_cb, this);
	lo_server_add_method(_server, "/om/synth/create_node", "issssi", create_node_cb, this);
	lo_server_add_method(_server, "/om/synth/create_node", "isssi", create_node_by_uri_cb, this);
	lo_server_add_method(_server, "/om/synth/destroy", "is", destroy_cb, this);
	lo_server_add_method(_server, "/om/synth/rename", "iss", rename_cb, this);
	lo_server_add_method(_server, "/om/synth/connect", "iss", connect_cb, this);
	lo_server_add_method(_server, "/om/synth/disconnect", "iss", disconnect_cb, this);
	lo_server_add_method(_server, "/om/synth/disconnect_all", "is", disconnect_all_cb, this);
	lo_server_add_method(_server, "/om/synth/set_port_value", "isf", set_port_value_cb, this);
	lo_server_add_method(_server, "/om/synth/set_port_value", "isif", set_port_value_voice_cb, this);
	lo_server_add_method(_server, "/om/synth/set_port_value_slow", "isf", set_port_value_slow_cb, this);
	lo_server_add_method(_server, "/om/synth/note_on", "isii", note_on_cb, this);
	lo_server_add_method(_server, "/om/synth/note_off", "isi", note_off_cb, this);
	lo_server_add_method(_server, "/om/synth/all_notes_off", "isi", all_notes_off_cb, this);
	lo_server_add_method(_server, "/om/synth/midi_learn", "is", midi_learn_cb, this);
#ifdef HAVE_LASH
	lo_server_add_method(_server, "/om/lash/restore_finished", "i", lash_restore_done_cb, this);
#endif
	
	lo_server_add_method(_server, "/om/metadata/request", "isss", metadata_get_cb, this);
	lo_server_add_method(_server, "/om/metadata/set", "isss", metadata_set_cb, this);
	
	// Queries
	lo_server_add_method(_server, "/om/request/plugins", "i", request_plugins_cb, this);
	lo_server_add_method(_server, "/om/request/all_objects", "i", request_all_objects_cb, this);
	lo_server_add_method(_server, "/om/request/port_value", "is", request_port_value_cb, this);
	
	// DSSI support
#ifdef HAVE_DSSI
	// XXX WARNING: notice this is a catch-all
	lo_server_add_method(_server, NULL, NULL, dssi_cb, this);
#endif

	lo_server_add_method(_server, NULL, NULL, unknown_cb, NULL);
}


OSCEngineReceiver::~OSCEngineReceiver()
{
	deactivate();

	if (_server != NULL)  {
		lo_server_free(_server);
		_server = NULL;
	}
}


void
OSCEngineReceiver::activate()
{
	set_name("OSCEngineReceiver");
	QueuedEventSource::activate();
	set_scheduling(SCHED_FIFO, 10);
}


void
OSCEngineReceiver::deactivate()
{
	cout << "[OSCEngineReceiver] Stopped OSC listening thread" << endl;
	QueuedEventSource::deactivate();
}


/** Override the semaphore driven _run method of QueuedEngineInterface
 * to wait on OSC messages and prepare them right away in the same thread.
 */
void
OSCEngineReceiver::_run()
{
	/* get a timestamp here and stamp all the events with the same time so
	 * they all get executed in the same cycle */

	while (true) {
		assert( ! unprepared_events());
		
		// Wait on a message and enqueue it
		lo_server_recv(_server);

		// Enqueue every other message that is here "now"
		// (would this provide truly atomic bundles?)
		while (lo_server_recv_noblock(_server, 0) > 0) ;

		// Process them all
		while (unprepared_events())
			_whipped(); // Whip our slave self

		// No more unprepared events
	}	
}


/** Create a new responder for this message, if necessary.
 *
 * This is based on the fact that the current responder is stored in a ref
 * counted pointer, and events just take a reference to that.  Thus, events
 * may delete their responder if we've since switched to a new one, or the
 * same one can stay around and serve a series of events.  Reference counting
 * is pretty sweet, eh?
 *
 * If this message came from the same source as the last message, no allocation
 * of responders or lo_addresses or any of it needs to be done.  Unfortunately
 * the only way to check is by comparing URLs, because liblo addresses suck.
 *
 * Really, this entire thing is a basically just a crafty way of partially
 * working around the fact that liblo addresses really suck.  Oh well.
 */
int
OSCEngineReceiver::set_response_address_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg, void* user_data)
{
	OSCEngineReceiver* const me = reinterpret_cast<OSCEngineReceiver*>(user_data);

	//cerr << "SET RESPONSE\n";

	if (argc < 1 || types[0] != 'i') // Not a valid Ingen message
		return 0; // Save liblo the trouble

	//cerr << "** valid msg\n";

	const int id = argv[0]->i;

	// Need to respond
	if (id != -1) {
		const lo_address addr = lo_message_get_source(msg);
		char* const      url  = lo_address_get_url(addr);
	
		//cerr << "** need to respond\n";

		// Currently have an OSC responder, check if it's still okay
		if (me->_responder == me->_osc_responder) {
			//cerr << "** osc responder\n";
		
			if (!strcmp(url, me->_osc_responder->url())) {
				// Nice one, same address
				//cerr << "** Using cached response address, hooray" << endl;
			} else {
				// Shitty deal, make a new one
				//cerr << "** Setting response address to " << url << "(2)" << endl;
				me->_osc_responder = CountedPtr<OSCResponder>(new OSCResponder(id, url));
				me->set_responder(me->_osc_responder);
				// (responder takes ownership of url, no leak)
			}
		
		// Otherwise we have a NULL responder, definitely need to set a new one
		} else {
			//cerr << "** null responder\n";
			me->_osc_responder = CountedPtr<OSCResponder>(new OSCResponder(id, url));
			me->set_responder(me->_osc_responder);
			//cerr << "** Setting response address to " << url << "(2)" << endl;
		}
	
	// Don't respond
	} else {
		me->disable_responses();
		//cerr << "** Not responding." << endl;
	}

	// If this returns 0 no OSC commands will work
	return 1;
}


void
OSCEngineReceiver::error_cb(int num, const char* msg, const char* path)
{
	cerr << "liblo server error " << num << " in path \"" << "\" - " << msg << endl;
}


/** \page engine_osc_namespace
 * <p> \b /om/ping - Immediately sends a successful response to the given response id.
 * \arg \b response-id (integer) </p> \n \n
 */
int
OSCEngineReceiver::m_ping_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	_responder->respond_ok();
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /om/ping_slow - Sends response after going through the ("slow") event queue.
 * \arg \b response-id (integer)
 *
 * \li See the documentation for /om/synth/set_port_value_slow for an explanation of how
 * this differs from /om/ping.  This is useful to send after sending a large cluster of
 * events as a sentinel and wait on it's response, to know when the events are all
 * finished processing.</p> \n \n
 */
int
OSCEngineReceiver::m_ping_slow_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	ping();
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /om/engine/quit - Terminates the engine.
 * \arg \b response-id (integer)
 *
 * \li Note that there is NO order guarantees with this command at all.  You could
 * send 10 messages then quit, and the quit reply could come immediately and the
 * 10 messages would never get executed. </p> \n \n
 */
int
OSCEngineReceiver::m_quit_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{

	quit();
	return 0;
}

/** \page engine_osc_namespace
 * <p> \b /om/engine/register_client - Registers a new client with the engine
 * \arg \b response-id (integer)
 *
 * The incoming address will be used for the new registered client.  If you
 * want to register a different specific address, use the URL version.
 */
int
OSCEngineReceiver::m_register_client_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	lo_address        addr         = lo_message_get_source(msg);

	char* const url = lo_address_get_url(addr);
	CountedPtr<ClientInterface> client(new OSCClientSender((const char*)url));
	register_client(ClientKey(ClientKey::OSC_URL, (const char*)url), client);
	free(url);

	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /om/engine/unregister_client - Unregisters a client
 * \arg \b response-id (integer) </p> \n \n
 */
int
OSCEngineReceiver::m_unregister_client_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	lo_address        addr         = lo_message_get_source(msg);

	char* url = lo_address_get_url(addr);
	unregister_client(ClientKey(ClientKey::OSC_URL, url));
	free(url);
	
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /om/engine/load_plugins - Locates all available plugins, making them available for use.
 * \arg \b response-id (integer) </p> \n \n
 */
int
OSCEngineReceiver::m_load_plugins_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	load_plugins();
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /om/engine/activate - Activate the engine (MIDI, audio, everything)
 * \arg \b response-id (integer) </p>
 *
 * \li Note that you <b>must</b> send this message first if you want the engine to do
 * anything at all - <em>including respond to your messages!</em> \n \n
 */
int
OSCEngineReceiver::m_engine_activate_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	QueuedEngineInterface::activate();
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /om/engine/deactivate - Deactivate the engine completely.
 * \arg \b response-id (integer) </p> \n \n
 */
int
OSCEngineReceiver::m_engine_deactivate_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	QueuedEngineInterface::deactivate();
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /om/synth/create_patch - Creates a new, empty, toplevel patch.
 * \arg \b response-id  (integer)
 * \arg \b patch-path  (string) - Patch path (complete, ie /master/parent/new_patch)
 * \arg \b poly        (integer) - Patch's (internal) polyphony </p> \n \n
 */
int
OSCEngineReceiver::m_create_patch_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* patch_path  = &argv[1]->s;
	const int   poly        =  argv[2]->i;

	create_patch(patch_path, poly);
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /om/synth/rename - Rename an Object (only Nodes, for now)
 * \arg \b response-id (integer)
 * \arg \b path - Object's path
 * \arg \b name - New name for object </p> \n \n
 */
int
OSCEngineReceiver::m_rename_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* object_path  = &argv[1]->s;
	const char* name         = &argv[2]->s;
	
	rename(object_path, name);
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /om/synth/enable_patch - Enable DSP processing of a patch
 * \arg \b response-id (integer)
 * \arg \b patch-path - Patch's path
 */
int
OSCEngineReceiver::m_enable_patch_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* patch_path  = &argv[1]->s;
	
	enable_patch(patch_path);
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /om/synth/disable_patch - Disable DSP processing of a patch
 * \arg \b response-id (integer)
 * \arg \b patch-path - Patch's path
 */
int
OSCEngineReceiver::m_disable_patch_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* patch_path  = &argv[1]->s;
	
	disable_patch(patch_path);
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /om/synth/clear_patch - Remove all nodes from a patch
 * \arg \b response-id (integer)
 * \arg \b patch-path - Patch's path
 */
int
OSCEngineReceiver::m_clear_patch_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* patch_path  = &argv[1]->s;
	
	clear_patch(patch_path);
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /om/synth/create_port - Add a port into a given patch (load a plugin by URI)
 * \arg \b response-id (integer)
 * \arg \b path (string) - Full path of the new port (ie. /patch2/subpatch/newport)
 * \arg \b data-type (string) - Data type for port to contain ("AUDIO", "CONTROL", or "MIDI")
 * \arg \b direction ("is-output") (integer) - Direction of data flow (Input = 0, Output = 1) </p> \n \n
 */
int
OSCEngineReceiver::m_create_port_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* port_path   = &argv[1]->s;
	const char* data_type   = &argv[2]->s;
	const int   direction   =  argv[3]->i;
	
	create_port(port_path, data_type, (direction == 1));
	return 0;
}

/** \page engine_osc_namespace
 * <p> \b /om/synth/create_node - Add a node into a given patch (load a plugin by URI)
 * \arg \b response-id (integer)
 * \arg \b node-path (string) - Full path of the new node (ie. /patch2/subpatch/newnode)
 * \arg \b type (string) - Plugin type ("Internal", "LV2", "DSSI", "LADSPA")
 * \arg \b plug-uri (string) - URI of the plugin to load
 * \arg \b poly (integer-boolean) - Whether node is polyphonic (0 = false, 1 = true) </p> \n \n
 */
int
OSCEngineReceiver::m_create_node_by_uri_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* node_path   = &argv[1]->s;
	const char* type        = &argv[2]->s;
	const char* plug_uri    = &argv[3]->s;
	const int   poly        =  argv[4]->i;
	
	// FIXME: make sure poly is valid
	
	create_node(node_path, type, plug_uri, (poly == 1));
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /om/synth/create_node - Add a node into a given patch (load a plugin by libname, label) \b DEPRECATED
 * \arg \b response-id (integer)
 * \arg \b node-path (string) - Full path of the new node (ie. /patch2/subpatch/newnode)
 * \arg \b type (string) - Plugin type ("LADSPA" or "Internal")
 * \arg \b lib-name (string) - Name of library where plugin resides (eg "cmt.so")
 * \arg \b plug-label (string) - Label (ID) of plugin (eg "sine_fcaa")
 * \arg \b poly (integer-boolean) - Whether node is polyphonic (0 = false, 1 = true)
 *
 * \li This is only here to provide backwards compatibility for old patches that store LADSPA plugin
 * references as libname, label.  It is to be removed ASAP, don't use it.
 * </p> \n \n
 */
int
OSCEngineReceiver::m_create_node_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* node_path   = &argv[1]->s;
	const char* type        = &argv[2]->s;
	const char* lib_name    = &argv[3]->s;
	const char* plug_label  = &argv[4]->s;
	const int   poly        =  argv[5]->i;
	
	create_node(node_path, type, lib_name, plug_label, (poly == 1));
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /om/synth/destroy - Removes (destroys) a Patch or a Node
 * \arg \b response-id (integer)
 * \arg \b node-path (string) - Full path of the object </p> \n \n
 */
int
OSCEngineReceiver::m_destroy_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* node_path   = &argv[1]->s;
	
	destroy(node_path);
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /om/synth/connect - Connects two ports (must be in the same patch)
 * \arg \b response-id (integer)
 * \arg \b src-port-path (string) - Full path of source port
 * \arg \b dst-port-path (string) - Full path of destination port </p> \n \n
 */
int
OSCEngineReceiver::m_connect_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* src_port_path = &argv[1]->s;
	const char* dst_port_path = &argv[2]->s;

	connect(src_port_path, dst_port_path);
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /om/synth/disconnect - Disconnects two ports.
 * \arg \b response-id (integer)
 * \arg \b src-port-path (string) - Full path of source port
 * \arg \b dst-port-path (string) - Full path of destination port </p> \n \n
 */
int
OSCEngineReceiver::m_disconnect_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* src_port_path = &argv[1]->s;
	const char* dst_port_path = &argv[2]->s;

	disconnect(src_port_path, dst_port_path);
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /om/synth/disconnect_all - Disconnect all connections to/from a node.
 * \arg \b response-id (integer)
 * \arg \b node-path (string) - Full path of node. </p> \n \n
 */
int
OSCEngineReceiver::m_disconnect_all_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* node_path   = &argv[1]->s;

	disconnect_all(node_path);
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /om/synth/set_port_value - Sets the value of a port for all voices (both AR and CR)
 * \arg \b response-id (integer)
 * \arg \b port-path (string) - Name of port
 * \arg \b value (float) - Value to set port to
 */
int
OSCEngineReceiver::m_set_port_value_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* port_path   = &argv[1]->s;
	const float value       =  argv[2]->f;

	set_port_value(port_path, value);
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /om/synth/set_port_value - Sets the value of a port for a specific voice (both AR and CR)
 * \arg \b response-id (integer)
 * \arg \b port-path (string) - Name of port
 * \arg \b voice (integer) - Voice to set port value for
 * \arg \b value (float) - Value to set port to
 */
int
OSCEngineReceiver::m_set_port_value_voice_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* port_path   = &argv[1]->s;
	const int   voice       =  argv[2]->i;
	const float value       =  argv[3]->f;

	set_port_value(port_path, voice, value);
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /om/synth/set_port_value_slow - Sets the value of a port for all voices (as a QueuedEvent)
 * \arg \b response-id (integer)
 * \arg \b port-path (string) - Name of port
 * \arg \b value (float) - Value to set port to
 *
 * \li This version exists so you can send it immediately after a QueuedEvent it may depend on (ie a
 * node creation) and be sure it happens after the event (a normal set_port_value could beat the
 * slow event and arrive out of order). </p> \n \n
 */
int
OSCEngineReceiver::m_set_port_value_slow_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* port_path   = &argv[1]->s;
	const float value       =  argv[2]->f;

	set_port_value_queued(port_path, value);
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /om/synth/note_on - Triggers a note-on, just as if it came from MIDI
 * \arg \b response-id (integer)
 * \arg \b node-path (string) - Patch of Node to trigger (must be a trigger or note node)
 * \arg \b note-num (int) - MIDI style note number (0-127)
 * \arg \b velocity (int) - MIDI style velocity (0-127)</p> \n \n
 */
int
OSCEngineReceiver::m_note_on_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	/*

	const char* node_path   = &argv[1]->s;
	const uchar note_num    =  argv[2]->i;
	const uchar velocity    =  argv[3]->i;
	*/
	cerr << "FIXME: OSC note on\n";
	//note_on(node_path, note_num, velocity);
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /om/synth/note_off - Triggers a note-off, just as if it came from MIDI
 * \arg \b response-id (integer)
 * \arg \b node-path (string) - Patch of Node to trigger (must be a trigger or note node)
 * \arg \b note-num (int) - MIDI style note number (0-127)</p> \n \n
 */
int
OSCEngineReceiver::m_note_off_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	/*

	const char* patch_path  = &argv[1]->s;
	const uchar note_num    =  argv[2]->i;
	*/
	cerr << "FIXME: OSC note off\n";
	//note_off(patch_path, note_num);
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /om/synth/all_notes_off - Triggers a note-off for all voices, just as if it came from MIDI
 * \arg \b response-id (integer)
 * \arg \b patch-path (string) - Patch of patch to send event to </p> \n \n
 */
int
OSCEngineReceiver::m_all_notes_off_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	/*

	const char* patch_path  = &argv[1]->s;
	*/
	cerr << "FIXME: OSC all notes off\n";
	//all_notes_off(patch_path);
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /om/synth/midi_learn - Initiate MIDI learn for a given (MIDI Control) Node
 * \arg \b response-id (integer)
 * \arg \b node-path (string) - Patch of the Node that should learn the next MIDI event.
 * 
 * \li This of course will only do anything for MIDI control nodes.  The node will learn the next MIDI
 * event that arrives at it's MIDI input port - no behind the scenes voodoo happens here.  It is planned
 * that a plugin specification supporting arbitrary OSC commands for plugins will exist one day, and this
 * method will go away completely. </p> \n \n
 */
int
OSCEngineReceiver::m_midi_learn_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* patch_path  = &argv[1]->s;
	
	midi_learn(patch_path);
		
	return 0;
}


#ifdef HAVE_LASH
/** \page engine_osc_namespace
 * <p> \b /om/lash/restore_done - Notify LASH restoring is finished and connections can be made.
 * \arg \b response-id (integer)
 */
int
OSCEngineReceiver::m_lash_restore_done_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	lash_restore_done();
	return 0;
}
#endif // HAVE_LASH


/** \page engine_osc_namespace
 * <p> \b /om/metadata/set - Sets a piece of metadata, associated with a synth-space object (node, etc)
 * \arg \b response-id (integer)
 * \arg \b object-path (string) - Full path of object to associate metadata with
 * \arg \b key (string) - Key (index) for new piece of metadata
 * \arg \b value (string) - Value of new piece of metadata
 */
int
OSCEngineReceiver::m_metadata_set_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* node_path   = &argv[1]->s;
	const char* key         = &argv[2]->s;
	const char* value       = &argv[3]->s;
	
	set_metadata(node_path, key, value);
	
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /om/metadata/responder - Requests the engine send a piece of metadata, associated with a synth-space object (node, etc)
 * \arg \b response-id (integer)
 * \arg \b object-path (string) - Full path of object metadata is associated with
 * \arg \b key (string) - Key (index) for piece of metadata
 *
 * \li Reply will be sent to client registered with the source address of this message.</p> \n \n
 */
int
OSCEngineReceiver::m_metadata_get_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	/*
	const char* node_path   = &argv[1]->s;
	const char* key         = &argv[2]->s;
	*/
	cerr << "FIXME: OSC metadata request\n";
	// FIXME: Equivalent?
	/*
	RequestMetadataEvent* ev = new RequestMetadataEvent(
		new OSCResponder(ClientKey(addr)),
		node_path, key);

		*/
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /om/responder/plugins - Requests the engine send a list of all known plugins.
 * \arg \b response-id (integer)
 *
 * \li Reply will be sent to client registered with the source address of this message.</p> \n \n
 */
int
OSCEngineReceiver::m_request_plugins_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	request_plugins();
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /om/responder/all_objects - Requests the engine send information about \em all objects (patches, nodes, etc)
 * \arg \b response-id (integer)
 * 
 * \li Reply will be sent to client registered with the source address of this message.</p> \n \n
 */
int
OSCEngineReceiver::m_request_all_objects_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	request_all_objects();
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /om/responder/port_value - Requests the engine send the value of a port.
 * \arg \b response-id (integer)
 * \arg \b port-path (string) - Full path of port to send the value of </p> \n \n
 *
 * \li Reply will be sent to client registered with the source address of this message.</p> \n \n
 */
int
OSCEngineReceiver::m_request_port_value_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* port_path = &argv[1]->s;

	request_port_value(port_path);
	return 0;
}


#ifdef HAVE_DSSI
int
OSCEngineReceiver::m_dssi_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
#if 0
	string node_path(path);

	if (node_path.substr(0, 5) != "/dssi")
		return 1;
	
	string command = node_path.substr(node_path.find_last_of("/")+1);
	node_path = node_path.substr(5); // chop off leading "/dssi/"
	node_path = node_path.substr(0, node_path.find_last_of("/")); // chop off command at end
	
	//cout << "DSSI:  Got message " << command << " for node " << node_path << endl;

	QueuedEvent* ev = NULL;
	
	if (command == "update" && !strcmp(types, "s"))
		ev = new DSSIUpdateEvent(NULL, node_path, &argv[0]->s);
	else if (command == "control" && !strcmp(types, "if"))
		ev = new DSSIControlEvent(NULL, node_path, argv[0]->i, argv[1]->f);
	else if (command == "configure" && ~strcmp(types, "ss"))
		ev = new DSSIConfigureEvent(NULL, node_path, &argv[0]->s, &argv[1]->s);
	else if (command == "program" && ~strcmp(types, "ii"))
		ev = new DSSIProgramEvent(NULL, node_path, argv[0]->i, argv[1]->i);

	if (ev != NULL)
		push(ev);
	else
		cerr << "[OSCEngineReceiver] Unknown DSSI command received: " << path << endl;
#endif
	return 0;
}
#endif


//  Static Callbacks //


// Display incoming OSC messages (for debugging purposes)
int
OSCEngineReceiver::generic_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg, void* user_data)
{
    printf("[OSCMsg] %s\n", path);
    
	for (int i=0; i < argc; ++i) {
		printf("         '%c'  ", types[i]);
		lo_arg_pp(lo_type(types[i]), argv[i]);
		printf("\n");
    }
    printf("\n");

	return 1;  // not handled
}


int
OSCEngineReceiver::unknown_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg, void* user_data)
{
	const lo_address addr = lo_message_get_source(msg);
	char* const      url  = lo_address_get_url(addr);

	cerr << "Unknown command " << path << " (" << types << "), sending error.\n";

	string error_msg = "Unknown command: ";
	error_msg.append(path).append(" ").append(types);

	OSCResponder(0, url).respond_error(error_msg);

	return 0;
}


} // namespace Ingen
