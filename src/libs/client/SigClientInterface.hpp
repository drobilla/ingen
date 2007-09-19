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
#include "interface/ClientInterface.hpp"
using std::string;

namespace Ingen {
namespace Client {


/** A LibSigC++ signal emitting interface for clients to use.
 *
 * This simply emits an sigc signal for every event (eg OSC message) coming from
 * the engine.  Use Store (which extends this) if you want a nice client-side
 * model of the engine.
 *
 * The signals here match the calls to ClientInterface exactly.  See the
 * documentation for ClientInterface for meanings of signal parameters.
 */
class SigClientInterface : virtual public Ingen::Shared::ClientInterface, public sigc::trackable
{
public:

	// Signal parameters match up directly with ClientInterface calls
	
	sigc::signal<void, int32_t>                            response_ok_sig;
	sigc::signal<void, int32_t, string>                    response_error_sig;
	sigc::signal<void>                                     bundle_begin_sig; 
	sigc::signal<void>                                     bundle_end_sig; 
	sigc::signal<void, string>                             error_sig; 
	sigc::signal<void, uint32_t>                           num_plugins_sig; 
	sigc::signal<void, string, string, string>             new_plugin_sig; 
	sigc::signal<void, string, uint32_t>                   new_patch_sig; 
	sigc::signal<void, string, string, bool, uint32_t>     new_node_sig; 
	sigc::signal<void, string, string, bool>               new_port_sig; 
	sigc::signal<void, string>                             patch_enabled_sig; 
	sigc::signal<void, string>                             patch_disabled_sig; 
	sigc::signal<void, string, uint32_t>                   patch_polyphony_sig; 
	sigc::signal<void, string>                             patch_cleared_sig; 
	sigc::signal<void, string, string>                     object_renamed_sig; 
	sigc::signal<void, string>                             object_destroyed_sig; 
	sigc::signal<void, string, string>                     connection_sig; 
	sigc::signal<void, string, string>                     disconnection_sig; 
	sigc::signal<void, string, string, Raul::Atom>         metadata_update_sig; 
	sigc::signal<void, string, float>                      control_change_sig; 
	sigc::signal<void, string, uint32_t, uint32_t, string> program_add_sig; 
	sigc::signal<void, string, uint32_t, uint32_t>         program_remove_sig; 

protected:

	// ClientInterface hooks that fire the above signals
	
	// FIXME: implement for this (is implemented for ThreadedSigClientInterface)
	void enable()  { }
	void disable() { }

	void bundle_begin() {}
	void bundle_end()   {}
	
	void transfer_begin() {}
	void transfer_end()   {}

	void num_plugins(uint32_t num) { num_plugins_sig.emit(num); }

	void response_ok(int32_t id)
		{ response_ok_sig.emit(id); }

	void response_error(int32_t id, const string& msg)
		{ response_error_sig.emit(id, msg); }
	
	void error(const string& msg)
		{ error_sig.emit(msg); }
	
	void new_plugin(const string& uri, const string& type_uri, const string& name)
		{ new_plugin_sig.emit(uri, type_uri, name); }
	
	void new_patch(const string& path, uint32_t poly)
		{ new_patch_sig.emit(path, poly); }
	
	void new_node(const string& plugin_uri, const string& node_path, bool is_polyphonic, uint32_t num_ports)
		{ new_node_sig.emit(plugin_uri, node_path, is_polyphonic, num_ports); }
	
	void new_port(const string& path, const string& data_type, bool is_output)
		{ new_port_sig.emit(path, data_type, is_output); }

	void connection(const string& src_port_path, const string& dst_port_path)
		{ connection_sig.emit(src_port_path, dst_port_path); }

	void object_destroyed(const string& path)
		{ object_destroyed_sig.emit(path); }
	
	void patch_enabled(const string& path)
		{ patch_enabled_sig.emit(path); }
	
	void patch_disabled(const string& path)
		{ patch_disabled_sig.emit(path); }
	
	void patch_polyphony(const string& path, uint32_t poly)
		{ patch_polyphony_sig.emit(path, poly); }

	void patch_cleared(const string& path)
		{ patch_cleared_sig.emit(path); }

	void object_renamed(const string& old_path, const string& new_path)
		{ object_renamed_sig.emit(old_path, new_path); }
	
	void disconnection(const string& src_port_path, const string& dst_port_path)
		{ disconnection_sig.emit(src_port_path, dst_port_path); }
	
	void metadata_update(const string& path, const string& key, const Raul::Atom& value)
		{ metadata_update_sig.emit(path, key, value); }

	void control_change(const string& port_path, float value)
		{ control_change_sig.emit(port_path, value); }

	void program_add(const string& path, uint32_t bank, uint32_t program, const string& name)
		{ program_add_sig.emit(path, bank, program, name); }
	
	void program_remove(const string& path, uint32_t bank, uint32_t program)
		{ program_remove_sig.emit(path, bank, program); }

protected:
	SigClientInterface() {}
};


} // namespace Client
} // namespace Ingen

#endif
