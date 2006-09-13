/* This file is part of Ingen. Copyright (C) 2006 Dave Robillard.
 * 
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef SIGCLIENTINTERFACE_H
#define SIGCLIENTINTERFACE_H

#include <inttypes.h>
#include <string>
#include <sigc++/sigc++.h>
#include "interface/ClientInterface.h"
using std::string;

namespace Ingen {
namespace Client {


/** A LibSigC++ signal emitting interface for clients to use.
 *
 * This simply emits an sigc signal for every event (eg OSC message) coming from
 * the engine.  Use Store (which extends this) if you want a nice client-side
 * model of the engine.
 */
class SigClientInterface : virtual public Ingen::Shared::ClientInterface, public sigc::trackable
{
public:

	// See the corresponding emitting functions below for parameter meanings
	
	sigc::signal<void, int32_t, bool, string>                  response_sig;
	sigc::signal<void>                                         bundle_begin_sig; 
	sigc::signal<void>                                         bundle_end_sig; 
	sigc::signal<void, string>                                 error_sig; 
	sigc::signal<void, uint32_t>                               num_plugins_sig; 
	sigc::signal<void, string, string, string>                 new_plugin_sig; 
	sigc::signal<void, string, uint32_t>                       new_patch_sig; 
	sigc::signal<void, string, string, string, bool, uint32_t> new_node_sig; 
	sigc::signal<void, string, string, bool>                   new_port_sig; 
	sigc::signal<void, string>                                 patch_enabled_sig; 
	sigc::signal<void, string>                                 patch_disabled_sig; 
	sigc::signal<void, string>                                 patch_cleared_sig; 
	sigc::signal<void, string, string>                         object_renamed_sig; 
	sigc::signal<void, string>                                 object_destroyed_sig; 
	sigc::signal<void, string, string>                         connection_sig; 
	sigc::signal<void, string, string>                         disconnection_sig; 
	sigc::signal<void, string, string, Atom>                   metadata_update_sig; 
	sigc::signal<void, string, float>                          control_change_sig; 
	sigc::signal<void, string, uint32_t, uint32_t, string>     program_add_sig; 
	sigc::signal<void, string, uint32_t, uint32_t>             program_remove_sig; 

protected:
	SigClientInterface() {}
};


} // namespace Client
} // namespace Ingen

#endif
