/* This file is part of IngenGtk.  Copyright (C) 2006 Dave Robillard.
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

#ifndef LASHCONTROLLER_H
#define LASHCONTROLLER_H

#include <string>
#include <lash/lash.h>
using std::string;

namespace Ingenuity {

class App;

/* Old and unused LASH controller.
 *
 * \ingroup Ingenuity
 */
class LashController
{
public:
	LashController(lash_args_t* args);
	~LashController();
	
	bool enabled() { return lash_enabled(_client); }
	void process_events();

private:
	void save(const string& dir);

	lash_client_t* _client;
	
	void handle_event(lash_event_t* conf);
	void handle_config(lash_config_t* conf);
};


} // namespace Ingenuity

#endif // LASHCONTROLLER_H
