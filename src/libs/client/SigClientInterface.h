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
	
	sigc::signal<void>                                                              bundle_begin_sig; 
	sigc::signal<void>                                                              bundle_end_sig; 
	sigc::signal<void, const string&>                                               error_sig; 
	sigc::signal<void, uint32_t>                                                    num_plugins_sig; 
	sigc::signal<void, const string&, const string&, const string&>                 new_plugin_sig; 
	sigc::signal<void, const string&, uint32_t>                                     new_patch_sig; 
	sigc::signal<void, const string&, const string&, const string&, bool, uint32_t> new_node_sig; 
	sigc::signal<void, const string&, const string&, bool>                          new_port_sig; 
	sigc::signal<void, const string&>                                               patch_enabled_sig; 
	sigc::signal<void, const string&>                                               patch_disabled_sig; 
	sigc::signal<void, const string&>                                               patch_cleared_sig; 
	sigc::signal<void, const string&, const string&>                                object_renamed_sig; 
	sigc::signal<void, const string&>                                               object_destroyed_sig; 
	sigc::signal<void, const string&, const string&>                                connection_sig; 
	sigc::signal<void, const string&, const string&>                                disconnection_sig; 
	sigc::signal<void, const string&, const string&, const string&>                 metadata_update_sig; 
	sigc::signal<void, const string&, float>                                        control_change_sig; 
	sigc::signal<void, const string&, uint32_t, uint32_t, const string&>            program_add_sig; 
	sigc::signal<void, const string&, uint32_t, uint32_t>                           program_remove_sig; 


	inline void emit_bundle_begin()
	{ bundle_begin_sig.emit(); }

	inline void emit_bundle_end()
	{ bundle_end_sig.emit(); }

	inline void emit_error(const string& msg)
	{ error_sig.emit(msg); }
	
	inline void emit_num_plugins(uint32_t num)
	{ num_plugins_sig.emit(num); }

	inline void emit_new_plugin(const string& type, const string& uri, const string& name)
	{ new_plugin_sig.emit(type, uri, name); }
	
	inline void emit_new_patch(const string& path, uint32_t poly)
	{ new_patch_sig.emit(path, poly); }
	
	inline void emit_new_node(const string& plugin_type, const string& plugin_uri, const string& node_path, bool is_polyphonic, uint32_t num_ports)
	{ new_node_sig.emit(plugin_type, plugin_uri, node_path, is_polyphonic, num_ports); }
	
	inline void emit_new_port(const string& path, const string& data_type, bool is_output)
	{ new_port_sig.emit(path, data_type, is_output); }
	
	inline void emit_patch_enabled(const string& path)
	{ patch_enabled_sig.emit(path); }
	
	inline void emit_patch_disabled(const string& path)
	{ patch_disabled_sig.emit(path); }
	
	inline void emit_patch_cleared(const string& path)
	{ patch_cleared_sig.emit(path); }
	
	inline void emit_object_renamed(const string& old_path, const string& new_path)
	{ object_renamed_sig.emit(old_path, new_path); }
	
	inline void emit_object_destroyed(const string& path)
	{ object_destroyed_sig.emit(path); }
	
	inline void emit_connection(const string& src_port_path, const string& dst_port_path)
	{ connection_sig.emit(src_port_path, dst_port_path); }
	
	inline void emit_disconnection(const string& src_port_path, const string& dst_port_path)
	{ disconnection_sig.emit(src_port_path, dst_port_path); }
	
	inline void emit_metadata_update(const string& subject_path, const string& predicate, const string& value)
	{ metadata_update_sig.emit(subject_path, predicate, value); }
	
	inline void emit_control_change(const string& port_path, float value)
	{ control_change_sig.emit(port_path, value); }
	
	inline void emit_program_add(const string& node_path, uint32_t bank, uint32_t program, const string& program_name)
	{ program_add_sig.emit(node_path, bank, program, program_name); }
	
	inline void emit_program_remove(const string& node_path, uint32_t bank, uint32_t program)
	{ program_remove_sig.emit(node_path, bank, program); }

protected:
	SigClientInterface() {}
};


} // namespace Client
} // namespace Ingen

#endif
