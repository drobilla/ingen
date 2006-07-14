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

#include "OSCModelEngineInterface.h"
#include <list>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include "OSCListener.h"
#include "PatchModel.h"
#include "ConnectionModel.h"
#include "PresetModel.h"
#include "ControlModel.h"
#include "NodeModel.h"
#include "PluginModel.h"

using std::cerr; using std::cout; using std::endl;

namespace LibOmClient {

	
/** Construct a OSCModelEngineInterface with a user-provided ModelClientInterface object for notification
 *  of engine events.
 */
OSCModelEngineInterface::OSCModelEngineInterface(const string& engine_url)
: OSCEngineInterface(engine_url),
  m_request_id(0),
  m_is_attached(false),
  m_is_registered(false)
  /*m_blocking(false),
  m_waiting_for_response(false),
  m_wait_response_id(0),
  m_response_received(false),
  m_wait_response_was_affirmative(false),
  m_response_semaphore(0)*/
{
}


OSCModelEngineInterface::~OSCModelEngineInterface()
{
	detach();
}


/** Attempt to connect to the Om engine and notify it of our existance.
 *
 * Passing a client_port of 0 will automatically choose a free port.  If the
 * @a block parameter is true, this function will not return until a connection
 * has successfully been made.
 */
void
OSCModelEngineInterface::attach(bool block)
{
	cerr << "FIXME: listen thread\n";
	//start_listen_thread(_client_port);
	
	/*if (engine_url == "") {
		string local_url = m_osc_listener->listen_url().substr(
			0, m_osc_listener->listen_url().find_last_of(":"));
		local_url.append(":16180");
		_engine_addr = lo_address_new_from_url(local_url.c_str());
	} else {
		_engine_addr = lo_address_new_from_url(engine_url.c_str());
	}
	*/
	_engine_addr = lo_address_new_from_url(_engine_url.c_str());

	if (_engine_addr == NULL) {
		cerr << "Unable to connect, aborting." << endl;
		exit(EXIT_FAILURE);
	}

	char* lo_url = lo_address_get_url(_engine_addr);
	cout << "[OSCModelEngineInterface] Attempting to contact engine at " << lo_url << " ..." << endl;

	this->ping();

	m_is_attached = true; // FIXME

	/*if (block) {
		set_wait_response_id(request_id);	

		while (1) {	
			if (m_response_semaphore.try_wait() != 0) {
				cout << ".";
				cout.flush();
				ping(request_id);
				usleep(100000);
			} else {
				cout << " connected." << endl;
				m_waiting_for_response = false;
				break;
			}
		}
	}
	*/

	free(lo_url);
}

void
OSCModelEngineInterface::detach()
{
	m_is_attached = false;
}

#if 0
void
OSCModelEngineInterface::start_listen_thread(int client_port)
{
	if (m_st != NULL)
		return;

	if (client_port == 0) { 
		m_st = lo_server_thread_new(NULL, error_cb);
	} else {
		char port_str[8];
		snprintf(port_str, 8, "%d", client_port);
		m_st = lo_server_thread_new(port_str, error_cb);
	}

	if (m_st == NULL) {
		cerr << "[OSCModelEngineInterface] Could not start OSC listener.  Aborting." << endl;
		exit(EXIT_FAILURE);
	} else {
		cout << "[OSCModelEngineInterface] Started OSC listener on port " << lo_server_thread_get_port(m_st) << endl;
	}

	lo_server_thread_add_method(m_st, NULL, NULL, generic_cb, NULL);

	lo_server_thread_add_method(m_st, "/om/response/ok", "i", om_response_ok_cb, this);
	lo_server_thread_add_method(m_st, "/om/response/error", "is", om_response_error_cb, this);
	
	
	m_osc_listener = new OSCListener(m_st, m_client_hooks);
	m_osc_listener->setup_callbacks();

	// Display any uncaught messages to the console
	lo_server_thread_add_method(m_st, NULL, NULL, unknown_cb, NULL);

	lo_server_thread_start(m_st);
}
#endif

///// OSC Commands /////



/** Load a node.
 */
void
OSCModelEngineInterface::create_node_from_model(const NodeModel* nm)
{
	assert(_engine_addr);
	
	// Load by URI
	if (nm->plugin()->uri().length() > 0) {
		lo_send(_engine_addr, "/om/synth/create_node",  "isssi", next_id(),
				nm->path().c_str(),
		        nm->plugin()->type_string(),
		        nm->plugin()->uri().c_str(),
				(nm->polyphonic() ? 1 : 0));
	// Load by libname, label
	} else {
		//assert(nm->plugin()->lib_name().length() > 0);
		assert(nm->plugin()->plug_label().length() > 0);
		lo_send(_engine_addr, "/om/synth/create_node",  "issssi", next_id(),
				nm->path().c_str(),
		        nm->plugin()->type_string(),
		        nm->plugin()->lib_name().c_str(),
		        nm->plugin()->plug_label().c_str(),
				(nm->polyphonic() ? 1 : 0));
	}
}


/** Create a patch.
 */
void
OSCModelEngineInterface::create_patch_from_model(const PatchModel* pm)
{
	assert(_engine_addr);
	lo_send(_engine_addr, "/om/synth/create_patch", "isi", next_id(), pm->path().c_str(), pm->poly());
}


/** Notify LASH restoring is finished */
/*
void
OSCModelEngineInterface::lash_restore_finished()
{
	assert(_engine_addr != NULL);
	int id = m_request_id++;
	lo_send(_engine_addr, "/om/lash/restore_finished", "i", id);
}
*/
#if 0
/** Set/add a piece of metadata.
 */
void
OSCModelEngineInterface::set_metadata(const string& obj_path,
                   const string& key, const string& value)
{
	assert(_engine_addr != NULL);
	int id = m_request_id++;

	// Deal with the "special" DSSI metadata strings
	if (key.substr(0, 16) == "dssi-configure--") {
		string path = "/dssi" + obj_path + "/configure";
		string dssi_key = key.substr(16);
		lo_send(_engine_addr, path.c_str(), "ss", dssi_key.c_str(), value.c_str());
	} else if (key == "dssi-program") {
		string path = "/dssi" + obj_path + "/program";
		string dssi_bank_str = value.substr(0, value.find("/"));
		int dssi_bank = atoi(dssi_bank_str.c_str());
		string dssi_program_str = value.substr(value.find("/")+1);
		int dssi_program = atoi(dssi_program_str.c_str());
		lo_send(_engine_addr, path.c_str(), "ii", dssi_bank, dssi_program);
	}
	
	// Normal metadata
	lo_send(_engine_addr, "/om/metadata/set", "isss", id,
		obj_path.c_str(), key.c_str(), value.c_str());
}
#endif

/** Set all pieces of metadata in a NodeModel.
 */
void
OSCModelEngineInterface::set_all_metadata(const NodeModel* nm)
{
	assert(_engine_addr != NULL);

	for (map<string, string>::const_iterator i = nm->metadata().begin(); i != nm->metadata().end(); ++i)
		set_metadata(nm->path(), (*i).first, (*i).second.c_str());
}


/** Set a preset by setting all relevant controls for a patch.
 */
void
OSCModelEngineInterface::set_preset(const string& patch_path, const PresetModel* const pm)
{
	assert(patch_path.find("//") == string::npos);
	for (list<ControlModel>::const_iterator i = pm->controls().begin(); i != pm->controls().end(); ++i) {
		set_port_value_queued((*i).port_path(), (*i).value());
		usleep(1000);
	}
}


///// Requests /////


#if 0
/** Sets the response ID to be waited for on the next call to wait_for_response()
 */

void
OSCModelEngineInterface::set_wait_response_id(int id)
{	
	assert(!m_waiting_for_response);
	m_wait_response_id = id;
	m_response_received = false;
	m_waiting_for_response = true;
}


/** Waits for the response set by set_wait_response() from the server.
 *
 * Returns whether or not the response was positive (ie a success message)
 * or negative (ie an error)
 */
bool
OSCModelEngineInterface::wait_for_response()
{
	cerr << "[OSCModelEngineInterface] Waiting for response " << m_wait_response_id << ": ";
	bool ret = true;
	
	assert(m_waiting_for_response);
	assert(!m_response_received);
	
	while (!m_response_received)
		m_response_semaphore.wait();
	
	cerr << " received." << endl;

	m_waiting_for_response = false;
	ret = m_wait_response_was_affirmative;
	
	return ret;
}
#endif

///// Static OSC callbacks //////


//// End static callbacks, member callbacks below ////

/*
int
OSCModelEngineInterface::m_om_response_ok_cb(const char* path, const char* types, lo_arg** argv, int argc, void* data)
{	
	assert(argc == 1 && !strcmp(types, "i"));
	
	// FIXME
	if (!m_is_attached)
		m_is_attached = true;

	if (m_waiting_for_response) {
		const int request_id = argv[0]->i;
	
		if (request_id == m_wait_response_id) {
			m_response_received = true;
			m_wait_response_was_affirmative = true;
			m_response_semaphore.post();
		}
	}

	return 0;
}


int
OSCModelEngineInterface::m_om_response_error_cb(const char* path, const char* types, lo_arg** argv, int argc, void* data)
{
	assert(argc == 2 && !strcmp(types, "is"));

	const int   request_id =  argv[0]->i;
	const char* msg        = &argv[1]->s;
	
	if (m_waiting_for_response) {
		if (request_id == m_wait_response_id) {
			m_response_received = true;
			m_wait_response_was_affirmative = false;
			m_response_semaphore.post();
		}
	}
	
	cerr << "ERROR: " << msg << endl;
	//if (m_client_hooks != NULL)
	//	m_client_hooks->error(msg);
	
	return 0;
}
*/

} // namespace LibOmClient
