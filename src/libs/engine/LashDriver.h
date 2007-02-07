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

#ifndef LASHDRIVER_H
#define LASHDRIVER_H

#include <lash/lash.h>
#include <pthread.h>
#include <string>
using std::string;

namespace Ingen {

class Ingen;


/** Handles all support for LASH session management.
 */
class LashDriver
{
public:
	LashDriver(Ingen* app, lash_args_t* args);

	bool enabled() { return (_client != NULL && lash_enabled(_client)); }
	void process_events();
	void set_jack_client_name(const char* name);
	void set_alsa_client_id(int id);
	void restore_finished();

private:
	Ingen*      _app;
	lash_client_t* _client;
	
	int    _alsa_client_id;
	string _jack_client_name;
	
	void handle_event(lash_event_t* conf);
	void handle_config(lash_config_t* conf);
};


} // namespace Ingen

#endif // LASHDRIVER_H
