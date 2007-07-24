/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#include <iostream>
#include <string>
#include <cassert>
#include "../../../../config/config.h"
#include "LashDriver.hpp"
#include "App.hpp"

using namespace std;

namespace Ingen {
	

LashDriver::LashDriver(Ingen* app, lash_args_t* args)
: _app(app),
  _client(NULL),
  _alsa_client_id(0) // FIXME: appropriate sentinel?
{
	_client = lash_init(args, PACKAGE_NAME,
		/*LASH_Config_Data_Set|LASH_Config_File*/0, LASH_PROTOCOL(2, 0));
	if (_client == NULL) {
		cerr << "[LashDriver] Failed to connect to LASH.  Session management will not function." << endl;
	} else {
		cout << "[LashDriver] Lash initialised" << endl;
		lash_event_t* event = lash_event_new_with_type(LASH_Client_Name);
		lash_event_set_string(event, "Ingen");
		lash_send_event(_client, event);			
	}
}


/** Set the Jack client name to be sent to LASH.
 * The name isn't actually send until restore_finished() is called.
 */
void
LashDriver::set_jack_client_name(const char* name)
{
	_jack_client_name = name;
	lash_jack_client_name(_client, _jack_client_name.c_str());
}


/** Set the Alsa client ID to be sent to LASH.
 * The name isn't actually send until restore_finished() is called.
 */
void
LashDriver::set_alsa_client_id(int id)
{
	_alsa_client_id = id;
	lash_alsa_client_id(_client, _alsa_client_id);
}


/** Notify LASH of our port names so it can restore connections.
 * The Alsa client ID and Jack client name MUST be set before calling
 * this function.
 */
void
LashDriver::restore_finished()
{
	assert(_jack_client_name != "");
	assert(_alsa_client_id != 0);

	cerr << "LASH RESTORE FINISHED " << _jack_client_name << "  -  " << _alsa_client_id << endl;

	lash_jack_client_name(_client, _jack_client_name.c_str());
	lash_alsa_client_id(_client, _alsa_client_id);
}


void
LashDriver::process_events()
{
	assert(_client != NULL);
	
	lash_event_t*  ev = NULL;
	lash_config_t* conf = NULL;

	// Process events
	while ((ev = lash_get_event(_client)) != NULL) {
		handle_event(ev);
		lash_event_destroy(ev);	
	}

	// Process configs
	while ((conf = lash_get_config(_client)) != NULL) {
		handle_config(conf);
		lash_config_destroy(conf);	
	}
}


void
LashDriver::handle_event(lash_event_t* ev)
{
	LASH_Event_Type type  = lash_event_get_type(ev);
	const char*     c_str = lash_event_get_string(ev);
	string          str   = (c_str == NULL) ? "" : c_str;
	
	//cout << "[LashDriver] LASH Event.  Type = " << type << ", string = " << str << "**********" << endl;
	
	/*if (type == LASH_Save_File) {
		//cout << "[LashDriver] LASH Save File - " << str << endl;
		_app->store_window_location();
		_app->state_manager()->save(str.append("/locations"));
		lash_send_event(_client, lash_event_new_with_type(LASH_Save_File));
	} else if (type == LASH_Restore_File) {
		//cout << "[LashDriver] LASH Restore File - " << str << endl;
		_app->state_manager()->load(str.append("/locations"));
		_app->update_state();
		lash_send_event(_client, lash_event_new_with_type(LASH_Restore_File));
	} else if (type == LASH_Save_Data_Set) {
		//cout << "[LashDriver] LASH Save Data Set - " << endl;
		
		// Tell LASH we're done
		lash_send_event(_client, lash_event_new_with_type(LASH_Save_Data_Set));
	} else*/
	if (type == LASH_Quit) {
		//stop_thread();
		_client = NULL;
		_app->quit();
	} else {
		cerr << "[LashDriver] WARNING: Unhandled lash event, type " << static_cast<int>(type) << endl;
	}
}


void
LashDriver::handle_config(lash_config_t* conf)
{
	const char*    key      = NULL;
	const void*    val      = NULL;
	size_t         val_size = 0;

	//cout << "[LashDriver] LASH Config.  Key = " << key << endl;

	key      = lash_config_get_key(conf);
	val      = lash_config_get_value(conf);
	val_size = lash_config_get_value_size(conf);
}


} // namespace Ingen
