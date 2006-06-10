/* This file is part of Om.  Copyright (C) 2006 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "LashDriver.h"
#include "config.h"
#include <iostream>
#include <string>
#include <cassert>
#include "OmApp.h"

using std::cerr; using std::cout; using std::endl;
using std::string;

namespace Om {
	

LashDriver::LashDriver(OmApp* app, lash_args_t* args)
: m_app(app),
  m_client(NULL),
  m_alsa_client_id(0) // FIXME: appropriate sentinel?
{
	m_client = lash_init(args, PACKAGE_NAME,
		/*LASH_Config_Data_Set|LASH_Config_File*/0, LASH_PROTOCOL(2, 0));
	if (m_client == NULL) {
		cerr << "[LashDriver] Failed to connect to LASH.  Session management will not function." << endl;
	} else {
		cout << "[LashDriver] Lash initialised" << endl;
		lash_event_t* event = lash_event_new_with_type(LASH_Client_Name);
		lash_event_set_string(event, "Om");
		lash_send_event(m_client, event);			
	}
}


/** Set the Jack client name to be sent to LASH.
 * The name isn't actually send until restore_finished() is called.
 */
void
LashDriver::set_jack_client_name(const char* name)
{
	m_jack_client_name = name;
	lash_jack_client_name(m_client, m_jack_client_name.c_str());
}


/** Set the Alsa client ID to be sent to LASH.
 * The name isn't actually send until restore_finished() is called.
 */
void
LashDriver::set_alsa_client_id(int id)
{
	m_alsa_client_id = id;
	lash_alsa_client_id(m_client, m_alsa_client_id);
}


/** Notify LASH of our port names so it can restore connections.
 * The Alsa client ID and Jack client name MUST be set before calling
 * this function.
 */
void
LashDriver::restore_finished()
{
	assert(m_jack_client_name != "");
	assert(m_alsa_client_id != 0);

	cerr << "LASH RESTORE FINISHED " << m_jack_client_name << "  -  " << m_alsa_client_id << endl;

	lash_jack_client_name(m_client, m_jack_client_name.c_str());
	lash_alsa_client_id(m_client, m_alsa_client_id);
}


void
LashDriver::process_events()
{
	assert(m_client != NULL);
	
	lash_event_t*  ev = NULL;
	lash_config_t* conf = NULL;

	// Process events
	while ((ev = lash_get_event(m_client)) != NULL) {
		handle_event(ev);
		lash_event_destroy(ev);	
	}

	// Process configs
	while ((conf = lash_get_config(m_client)) != NULL) {
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
		m_app->store_window_location();
		m_app->state_manager()->save(str.append("/locations"));
		lash_send_event(m_client, lash_event_new_with_type(LASH_Save_File));
	} else if (type == LASH_Restore_File) {
		//cout << "[LashDriver] LASH Restore File - " << str << endl;
		m_app->state_manager()->load(str.append("/locations"));
		m_app->update_state();
		lash_send_event(m_client, lash_event_new_with_type(LASH_Restore_File));
	} else if (type == LASH_Save_Data_Set) {
		//cout << "[LashDriver] LASH Save Data Set - " << endl;
		
		// Tell LASH we're done
		lash_send_event(m_client, lash_event_new_with_type(LASH_Save_Data_Set));
	} else*/
	if (type == LASH_Quit) {
		//stop_thread();
		m_client = NULL;
		m_app->quit();
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


} // namespace Om
