/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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

//#define ENABLE_AVAHI 1

#include <cstdlib>
#include <string>
#include <lo/lo.h>
#include "ingen-config.h"
#include "raul/log.hpp"
#include "raul/AtomLiblo.hpp"
#include "raul/SharedPtr.hpp"
#include "interface/ClientInterface.hpp"
#include "ClientBroadcaster.hpp"
#include "Engine.hpp"
#include "OSCClientSender.hpp"
#include "OSCEngineReceiver.hpp"
#include "EventSource.hpp"
#include "ThreadManager.hpp"

#define LOG(s) s << "[OSCEngineReceiver] "

using namespace std;
using namespace Raul;

namespace Ingen {

using namespace Shared;


/*! \page engine_osc_namespace Engine OSC Namespace Documentation
 *
 * <p>These are the commands the engine recognizes.  A client can control every
 * aspect of the engine entirely with these commands.</p>
 *
 * <p>All commands on this page are in the "control band".  If a client needs to
 * know about the state of the engine, it must listen to the "notification band".
 * See the "Client OSC Namespace Documentation" for details.
 */


OSCEngineReceiver::OSCEngineReceiver(Engine& engine, size_t queue_size, uint16_t port)
	: QueuedEngineInterface(engine, queue_size) // FIXME
	, _server(NULL)
{
	_receive_thread = new ReceiveThread(*this);

	char port_str[6];
	snprintf(port_str, 6, "%u", port);

	_server = lo_server_new(port_str, error_cb);
#ifdef ENABLE_AVAHI
	lo_server_avahi_init(_server, "ingen");
#endif

	if (_server == NULL) {
		LOG(error) << "Could not start OSC server.  Aborting." << endl;
		exit(EXIT_FAILURE);
	} else {
		char* lo_url = lo_server_get_url(_server);
		LOG(info) << "Started OSC server at " << lo_url << endl;
		free(lo_url);
	}

#ifdef LOG_DEBUG
	lo_server_add_method(_server, NULL, NULL, generic_cb, NULL);
#endif

	// Set response address for this message.
	// It's important this is first and returns nonzero.
	lo_server_add_method(_server, NULL, NULL, set_response_address_cb, this);

#ifdef LIBLO_BUNDLES
	lo_server_add_bundle_handler(_server, bundle_cb, this);
#endif

	// Commands
	lo_server_add_method(_server, "/ping", "i", ping_cb, this);
	lo_server_add_method(_server, "/ping_queued", "i", ping_slow_cb, this);
	lo_server_add_method(_server, "/quit", "i", quit_cb, this);
	lo_server_add_method(_server, "/register_client", "i", register_client_cb, this);
	lo_server_add_method(_server, "/unregister_client", "i", unregister_client_cb, this);
	lo_server_add_method(_server, "/load_plugins", "i", load_plugins_cb, this);
	lo_server_add_method(_server, "/activate", "i", engine_activate_cb, this);
	lo_server_add_method(_server, "/deactivate", "i", engine_deactivate_cb, this);
	lo_server_add_method(_server, "/put", NULL, put_cb, this);
	lo_server_add_method(_server, "/move", "iss", move_cb, this);
	lo_server_add_method(_server, "/delete", "is", del_cb, this);
	lo_server_add_method(_server, "/connect", "iss", connect_cb, this);
	lo_server_add_method(_server, "/disconnect", "iss", disconnect_cb, this);
	lo_server_add_method(_server, "/disconnect_all", "iss", disconnect_all_cb, this);
	lo_server_add_method(_server, "/note_on", "isii", note_on_cb, this);
	lo_server_add_method(_server, "/note_off", "isi", note_off_cb, this);
	lo_server_add_method(_server, "/all_notes_off", "isi", all_notes_off_cb, this);
	lo_server_add_method(_server, "/learn", "is", learn_cb, this);
	lo_server_add_method(_server, "/set_property", NULL, set_property_cb, this);

	// Queries
	lo_server_add_method(_server, "/request_property", "iss", request_property_cb, this);
	lo_server_add_method(_server, "/get", "is", get_cb, this);
	lo_server_add_method(_server, "/request_plugins", "i", request_plugins_cb, this);
	lo_server_add_method(_server, "/request_all_objects", "i", request_all_objects_cb, this);

	lo_server_add_method(_server, NULL, NULL, unknown_cb, NULL);

	Thread::set_name("OSCEngineReceiver");
}


OSCEngineReceiver::~OSCEngineReceiver()
{
	deactivate();
	stop();
	_receive_thread->stop();
	delete _receive_thread;

	if (_server != NULL)  {
#ifdef ENABLE_AVAHI
		lo_server_avahi_free(_server);
#endif
		lo_server_free(_server);
		_server = NULL;
	}
}


void
OSCEngineReceiver::activate_source()
{
	EventSource::activate_source();
	_receive_thread->set_name("OSCEngineReceiver Listener");
	_receive_thread->start();
	_receive_thread->set_scheduling(SCHED_FIFO, 5); // Jack default appears to be 10
}


void
OSCEngineReceiver::deactivate_source()
{
	_receive_thread->stop();
	EventSource::deactivate_source();
}


/** Override the semaphore driven _run method of QueuedEngineInterface
 * to wait on OSC messages and prepare them right away in the same thread.
 */
void
OSCEngineReceiver::ReceiveThread::_run()
{
	Thread::get().set_context(THREAD_PRE_PROCESS);

	/* get a timestamp here and stamp all the events with the same time so
	 * they all get executed in the same cycle */

	while (true) {
		// Wait on a message and enqueue it
		lo_server_recv(_receiver._server);

		// Enqueue every other message that is here "now"
		// (would this provide truly atomic bundles?)
		while (lo_server_recv_noblock(_receiver._server, 0) > 0)
			if (_receiver.unprepared_events())
				_receiver.whip();

		// No more unprepared events
	}
}


/** Create a new request for this message, if necessary.
 *
 * This is based on the fact that the current request is stored in a ref
 * counted pointer, and events just take a reference to that.  Thus, events
 * may delete their request if we've since switched to a new one, or the
 * same one can stay around and serve a series of events.
 * Hooray for reference counting.
 *
 * If this message came from the same source as the last message, no allocation
 * of requests or lo_addresses or any of it needs to be done.  Unfortunately
 * the only way to check is by comparing URLs, because liblo addresses suck.
 * Lack of a fast liblo address comparison really sucks here, in any case.
 */
int
OSCEngineReceiver::set_response_address_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg, void* user_data)
{
	OSCEngineReceiver* const me = reinterpret_cast<OSCEngineReceiver*>(user_data);

	if (argc < 1 || types[0] != 'i') // Not a valid Ingen message
		return 0; // Save liblo the trouble

	const int32_t id = argv[0]->i;

	const lo_address addr = lo_message_get_source(msg);
	char* const      url  = lo_address_get_url(addr);

	const SharedPtr<Request> r = me->_request;

	/* Different address than last time, have to do a lookup */
	if (!r || !r->client() || strcmp(url, r->client()->uri().c_str())) {
		Shared::ClientInterface* client = me->_engine.broadcaster()->client(url);
		if (client)
			me->_request = SharedPtr<Request>(new Request(me, client, id));
		else
			me->_request = SharedPtr<Request>(new Request(me));
	}

	if (id != -1) {
		me->set_next_response_id(id);
	} else {
		me->disable_responses();
	}

	free(url);

	// If this returns 0 no OSC commands will work
	return 1;
}


#ifdef LIBLO_BUNDLES
int
OSCEngineReceiver::_bundle_cb(lo_bundle_edge edge)
{
	switch (edge) {
	case LO_BUNDLE_BEGIN:
		info << "BUNDLE BEGIN" << endl;
		break;
	case LO_BUNDLE_END:
		info << "BUNDLE END" << endl;
		break;
	}
	return 0;
}
#endif


void
OSCEngineReceiver::error_cb(int num, const char* msg, const char* path)
{
	error << "liblo server error" << num;
	if (path) {
		error << " for path `" << path << "'";
	}
	error << " (" << msg << ")" << endl;
}


/** \page engine_osc_namespace
 * <h2>/ping</h2>
 * \arg \b response-id (integer)
 *
 * Reply to sender immediately with a successful response.
 */
int
OSCEngineReceiver::_ping_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const lo_address addr = lo_message_get_source(msg);
	if (lo_send(addr, "/ok", "i", argv[0]->i) < 0)
		warn << "Unable to send response (" << lo_address_errstr(addr) << ")" << endl;
	return 0;
}


/** \page engine_osc_namespace
 * <h2>/ping_queued</h2>
 * \arg \b response-id (integer)
 *
 * Reply to sender with a successful response after going through the event queue.
 * This is useful for checking if the engine is actually active, or for sending after
 * several events as a sentinel and wait on it's response, to know when all previous
 * events have finished processing.
 */
int
OSCEngineReceiver::_ping_slow_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	ping();
	return 0;
}


/** \page engine_osc_namespace
 * <h2>/quit</h2>
 * \arg \b response-id (integer)
 *
 * Terminate the engine.
 * Note that there are NO order guarantees with this command at all.  You could
 * send 10 messages then quit, and the quit reply could come immediately and the
 * 10 messages would never get executed.
 */
int
OSCEngineReceiver::_quit_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	quit();
	return 0;
}


/** \page engine_osc_namespace
 * <h2>/register_client</h2>
 * \arg \b response-id (integer)
 *
 * Register a new client with the engine.
 * The incoming address will be used for the new registered client.  If you
 * want to register a different specific address, use the URL version.
 */
int
OSCEngineReceiver::_register_client_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	lo_address addr = lo_message_get_source(msg);

	char* const url = lo_address_get_url(addr);
	Shared::ClientInterface* client = new OSCClientSender((const char*)url);
	register_client(client);
	free(url);

	return 0;
}


/** \page engine_osc_namespace
 * <h2>/unregister_client</h2>
 * \arg \b response-id (integer)
 *
 * Unregister a client.
 */
int
OSCEngineReceiver::_unregister_client_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	lo_address addr = lo_message_get_source(msg);

	char* url = lo_address_get_url(addr);
	unregister_client(url);
	free(url);

	return 0;
}


/** \page engine_osc_namespace
 * <h2>/load_plugins</h2>
 * \arg \b response-id (integer)
 *
 * Locate all available plugins, making them available for use.
 */
int
OSCEngineReceiver::_load_plugins_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	load_plugins();
	return 0;
}


/** \page engine_osc_namespace
 * <h2>/activate</h2>
 * \arg \b response-id (integer)
 *
 * Activate the engine (event processing and all drivers, e.g. audio and MIDI).
 * Note that you <b>must</b> send this message first if you want the engine to do
 * anything at all - <em>including respond to your messages!</em>
 */
int
OSCEngineReceiver::_engine_activate_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	QueuedEngineInterface::activate();
	return 0;
}


/** \page engine_osc_namespace
 * <h2>/deactivate</h2>
 * \arg \b response-id (integer)
 *
 * Deactivate the engine.
 */
int
OSCEngineReceiver::_engine_deactivate_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	QueuedEngineInterface::deactivate();
	return 0;
}


/** \page engine_osc_namespace
 * <h2>/get</h2>
 * \arg \b response-id (integer)
 * \arg \b uri (string) - URI of object (patch, port, node, plugin) to send
 *
 * Request all properties of an object.
 */
int
OSCEngineReceiver::_get_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	get(&argv[1]->s);
	return 0;
}


/** \page engine_osc_namespace
 * <h2>/put</h2>
 * \arg \b response-id (integer)
 * \arg \b path (string) - Path of object
 * \arg \b predicate
 * \arg \b value
 * \arg \b ...
 *
 * PUT a set of properties to a path (see \ref methods).
 */
int
OSCEngineReceiver::_put_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* obj_path = &argv[1]->s;
	Resource::Properties prop;
	for (int i = 2; i < argc-1; i += 2)
		prop.insert(make_pair(&argv[i]->s, AtomLiblo::lo_arg_to_atom(types[i+1], argv[i+1])));
	put(obj_path, prop);
	return 0;
}


/** \page engine_osc_namespace
 * <h2>/move</h2>
 * \arg \b response-id (integer)
 * \arg \b old-path - Object's path
 * \arg \b new-path - Object's new path
 *
 * MOVE an object to a new path (see \ref methods).
 */
int
OSCEngineReceiver::_move_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* old_path = &argv[1]->s;
	const char* new_path = &argv[2]->s;

	move(old_path, new_path);
	return 0;
}


/** \page engine_osc_namespace
 * <h2>/del</h2>
 * \arg \b response-id (integer)
 * \arg \b path (string) - Full path of the object
 *
 * DELETE an object (see \ref methods).
 */
int
OSCEngineReceiver::_del_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* obj_path = &argv[1]->s;

	del(obj_path);
	return 0;
}


/** \page engine_osc_namespace
 * <h2>/connect</h2>
 * \arg \b response-id (integer)
 * \arg \b src-port-path (string) - Full path of source port
 * \arg \b dst-port-path (string) - Full path of destination port
 *
 * Connect two ports (which must be in the same patch).
 */
int
OSCEngineReceiver::_connect_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* src_port_path = &argv[1]->s;
	const char* dst_port_path = &argv[2]->s;

	connect(src_port_path, dst_port_path);
	return 0;
}


/** \page engine_osc_namespace
 * <h2>/disconnect</h2>
 * \arg \b response-id (integer)
 * \arg \b src-port-path (string) - Full path of source port
 * \arg \b dst-port-path (string) - Full path of destination port
 *
 * Disconnect two ports.
 */
int
OSCEngineReceiver::_disconnect_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* src_port_path = &argv[1]->s;
	const char* dst_port_path = &argv[2]->s;

	disconnect(src_port_path, dst_port_path);
	return 0;
}


/** \page engine_osc_namespace
 * <h2>/disconnect_all</h2>
 * \arg \b response-id (integer)
 * \arg \b patch-path (string) - The (parent) patch in which to disconnect object.
 * \arg \b object-path (string) - Full path of object.
 *
 * Disconnect all connections to/from a node/port.
 */
int
OSCEngineReceiver::_disconnect_all_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* patch_path = &argv[1]->s;
	const char* object_path = &argv[2]->s;

	disconnect_all(patch_path, object_path);
	return 0;
}


/** \page engine_osc_namespace
 * <h2>/note_on</h2>
 * \arg \b response-id (integer)
 * \arg \b node-path (string) - Patch of Node to trigger (must be a trigger or note node)
 * \arg \b note-num (int) - MIDI style note number (0-127)
 * \arg \b velocity (int) - MIDI style velocity (0-127)
 *
 * Trigger a note-on, just as if it came from MIDI.
 */
int
OSCEngineReceiver::_note_on_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	/*
	const char* node_path   = &argv[1]->s;
	const uint8_t note_num    =  argv[2]->i;
	const uint8_t velocity    =  argv[3]->i;
	*/
	warn << "TODO: OSC note on" << endl;
	//note_on(node_path, note_num, velocity);
	return 0;
}


/** \page engine_osc_namespace
 * <h2>/note_off</h2>
 * \arg \b response-id (integer)
 * \arg \b node-path (string) - Patch of Node to trigger (must be a trigger or note node)
 * \arg \b note-num (int) - MIDI style note number (0-127)
 *
 * Trigger a note-off, just as if it came from MIDI.
 */
int
OSCEngineReceiver::_note_off_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	/*
	const char* patch_path  = &argv[1]->s;
	const uint8_t note_num    =  argv[2]->i;
	*/
	warn << "TODO: OSC note off" << endl;
	//note_off(patch_path, note_num);
	return 0;
}


/** \page engine_osc_namespace
 * <h2>/all_notes_off</h2>
 * \arg \b response-id (integer)
 * \arg \b patch-path (string) - Patch of patch to send event to
 *
 * Trigger a note-off for all voices, just as if it came from MIDI.
 */
int
OSCEngineReceiver::_all_notes_off_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	/*

	const char* patch_path  = &argv[1]->s;
	*/
	warn << "TODO: OSC all notes off" << endl;
	//all_notes_off(patch_path);
	return 0;
}


/** \page engine_osc_namespace
 * <h2>/set_property</h2>
 * \arg \b response-id (integer)
 * \arg \b object-path (string) - Full path of object to associate property with
 * \arg \b key (string) - URI for predicate of this property (e.g. "http://drobilla.net/ns/ingen#enabled")
 * \arg \b value (string) - Value of property
 *
 * Set a property on a graph object.
 */
int
OSCEngineReceiver::_set_property_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	if (argc != 4 || types[0] != 'i' || types[1] != 's' || types[2] != 's')
		return 1;

	const char* object_path = &argv[1]->s;
	const char* key         = &argv[2]->s;

	Raul::Atom value = Raul::AtomLiblo::lo_arg_to_atom(types[3], argv[3]);

	set_property(object_path, key, value);
	return 0;
}


/** \page engine_osc_namespace
 * <h2>/request_property</h2>
 * \arg \b response-id (integer)
 * \arg \b uri (string) - Subject
 * \arg \b key (string) - Predicate
 *
 * Request the value of a property on an object.
 */
int
OSCEngineReceiver::_request_property_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* object_path = &argv[1]->s;
	const char* key         = &argv[2]->s;

	request_property(object_path, key);
	return 0;
}


/** \page engine_osc_namespace
 * <h2>/request_plugins</h2>
 * \arg \b response-id (integer)
 *
 * Request the engine send a list of all known plugins.
 */
int
OSCEngineReceiver::_request_plugins_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	request_plugins();
	return 0;
}



//  Static Callbacks //


// Display incoming OSC messages (for debugging purposes)
int
OSCEngineReceiver::generic_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg, void* user_data)
{
	printf("[OSCEngineReceiver] %s (%s)\t", path, types);

	for (int i=0; i < argc; ++i) {
		lo_arg_pp(lo_type(types[i]), argv[i]);
		printf("\t");
    }
    printf("\n");

	return 1;  // not handled
}


int
OSCEngineReceiver::unknown_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg, void* user_data)
{
	const lo_address addr = lo_message_get_source(msg);
	char* const      url  = lo_address_get_url(addr);

	warn << "Unknown OSC command " << path << " (" << types << ")" << endl;

	string error_msg = "Unknown command: ";
	error_msg.append(path).append(" ").append(types);

	OSCClientSender(url).error(error_msg);
	free(url);

	return 0;
}


} // namespace Ingen
