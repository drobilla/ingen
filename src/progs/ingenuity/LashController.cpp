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

#include "LashController.h"
#include "config.h"
#include <iostream>
#include <string>
#include <cassert>
#include <sys/stat.h>
#include <sys/types.h>
#include "App.h"
#include "PatchModel.h"

using std::cerr; using std::cout; using std::endl;
using std::string;

namespace Ingenuity {
	

LashController::LashController(lash_args_t* args)
: _client(NULL)
{
	_client = lash_init(args, PACKAGE_NAME,
		/*LASH_Config_Data_Set|*/LASH_Config_File, LASH_PROTOCOL(2, 0));
	if (_client == NULL) {
		cerr << "[LashController] Failed to connect to LASH.  Session management will not function." << endl;
	} else {
		cout << "[LashController] Lash initialised" << endl;
	}
	
	lash_event_t* event = lash_event_new_with_type(LASH_Client_Name);
	lash_event_set_string(event, "Ingenuity");
	lash_send_event(_client, event);			
}


LashController::~LashController()
{
	if (_client != NULL) {
		lash_event_t* quit_event = lash_event_new_with_type(LASH_Quit);
		lash_send_event(_client, quit_event);
	}
}


void
LashController::process_events()
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
LashController::handle_event(lash_event_t* ev)
{
	assert(ev != NULL);
	
	LASH_Event_Type type  = lash_event_get_type(ev);
	const char*     c_str = lash_event_get_string(ev);
	string          str   = (c_str == NULL) ? "" : c_str;
	
	if (type == LASH_Save_File) {
		cout << "[LashController] LASH Save File - " << str << endl;
		save(str);
		lash_send_event(_client, lash_event_new_with_type(LASH_Save_File));
	} else if (type == LASH_Restore_File) {
		cout << "[LashController] LASH Restore File - " << str << endl;
		cerr << "LASH RESTORE NOT YET (RE)IMPLEMENTED." << endl;
		/*_controller->load_session_blocking(str + "/session");
		_controller->lash_restore_finished();
		lash_send_event(_client, lash_event_new_with_type(LASH_Restore_File));
		*/
	/*} else if (type == LASH_Save_Data_Set) {
		//cout << "[LashController] LASH Save Data Set - " << endl;
		
		// Tell LASH we're done
		lash_send_event(_client, lash_event_new_with_type(LASH_Save_Data_Set));
		*/
	} else if (type == LASH_Quit) {
		cout << "[LashController] LASH Quit" << endl;
		_client = NULL;
		App::instance().quit();
	} else {
		cerr << "[LashController] Unhandled LASH event, type: " << static_cast<int>(type) << endl;
	}
}


void
LashController::handle_config(lash_config_t* conf)
{
	assert(conf != NULL);
	
	const char*    key      = NULL;
	const void*    val      = NULL;
	size_t         val_size = 0;

	cout << "[LashController] LASH Config.  Key = " << key << endl;

	key      = lash_config_get_key(conf);
	val      = lash_config_get_value(conf);
	val_size = lash_config_get_value_size(conf);
}

void
LashController::save(const string& dir)
{
	cerr << "LASH SAVING NOT YET (RE)IMPLEMENTED\n";
	/*
	PatchController* pc = NULL;
	
	// Save every patch under dir with it's path as a filename
	// (so the dir structure will resemble the patch heirarchy)
	for (map<string,PatchController*>::iterator i = _app->patches().begin();
			i != _app->patches().end(); ++i) {
		pc = (*i).second;
		pc->model()->filename(dir + pc->model()->path() + ".om");
	}
	
	// Create directories
	for (map<string,PatchController*>::iterator i = _app->patches().begin();
			i != _app->patches().end(); ++i) {
		pc = (*i).second;
		if (pc->model()->parent() != NULL) {
			string path = Path::parent(pc->model()->path()).substr(1) + "/";
			while (path.find("/") != string::npos) {
				mkdir(string(dir +"/"+ path.substr(0, path.find("/"))).c_str(), 0744);
				path = path.substr(path.find("/")+1);
			}
		}
		_controller->save_patch_blocking(pc->model(), pc->model()->filename(), false);
	}
	
	//m_app->state_manager()->save(str + "/omgtkrc");
	_controller->save_session_blocking(dir + "/session");
	*/
}

} // namespace Ingenuity
