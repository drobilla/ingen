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

#ifndef THREADEDSIGCLIENTINTERFACE_H
#define THREADEDSIGCLIENTINTERFACE_H

#include <inttypes.h>
#include <string>
#include <sigc++/sigc++.h>
#include "interface/ClientInterface.h"
#include "SigClientInterface.h"
#include "util/Queue.h"
using std::string;

/** Returns nothing and takes no parameters (because they have all been bound) */
typedef sigc::slot<void> Closure;

namespace Ingen {
namespace Client {


/** A LibSigC++ signal emitting interface for clients to use.
 *
 * This emits signals (possibly) in a different thread than the ClientInterface
 * functions are called.  It must be explicitly driven with the emit_signals()
 * function, which fires all enqueued signals up until the present.  You can
 * use this in a GTK idle callback for receiving thread safe engine signals.
 */
class ThreadedSigClientInterface : virtual public SigClientInterface
{
public:
	ThreadedSigClientInterface(uint32_t queue_size)
	: _sigs(queue_size)
	, response_slot(response_sig.make_slot())
	, error_slot(error_sig.make_slot())
	, new_plugin_slot(new_plugin_sig.make_slot())
	, new_patch_slot(new_patch_sig.make_slot())
	, new_node_slot(new_node_sig.make_slot())
	, new_port_slot(new_port_sig.make_slot())
	, connection_slot(connection_sig.make_slot())
	, patch_enabled_slot(patch_enabled_sig.make_slot())
	, patch_disabled_slot(patch_disabled_sig.make_slot())
	, patch_cleared_slot(patch_cleared_sig.make_slot())
	, object_destroyed_slot(object_destroyed_sig.make_slot())
	, object_renamed_slot(object_renamed_sig.make_slot())
	, disconnection_slot(disconnection_sig.make_slot())
	, metadata_update_slot(metadata_update_sig.make_slot())
	, control_change_slot(control_change_sig.make_slot())
	, program_add_slot(program_add_sig.make_slot())
	, program_remove_slot(program_remove_sig.make_slot())
	{}


	// FIXME
	void bundle_begin() {}
	void bundle_end()   {}

	void num_plugins(uint32_t num) { _num_plugins = num; }

	void response(int32_t id, bool success, const string& msg)
		{ push_sig(sigc::bind(response_slot, id, success, msg)); }

	void error(const string& msg)
		{ push_sig(sigc::bind(error_slot, msg)); }
	
	void new_plugin(const string& type, const string& uri, const string& name)
		{ push_sig(sigc::bind(new_plugin_slot, type, uri, name)); }
	
	void new_patch(const string& path, uint32_t poly)
		{ push_sig(sigc::bind(new_patch_slot, path, poly)); }
	
	void new_node(const string& plugin_type, const string& plugin_uri, const string& node_path, bool is_polyphonic, uint32_t num_ports)
		{ push_sig(sigc::bind(new_node_slot, plugin_type, plugin_uri, node_path, is_polyphonic, num_ports)); }
	
	void new_port(const string& path, const string& data_type, bool is_output)
		{ push_sig(sigc::bind(new_port_slot, path, data_type, is_output)); }

	void connection(const string& src_port_path, const string& dst_port_path)
		{ push_sig(sigc::bind(connection_slot, src_port_path, dst_port_path)); }

	void object_destroyed(const string& path)
		{ push_sig(sigc::bind(object_destroyed_slot, path)); }
	
	void patch_enabled(const string& path)
		{ push_sig(sigc::bind(patch_enabled_slot, path)); }
	
	void patch_disabled(const string& path)
		{ push_sig(sigc::bind(patch_disabled_slot, path)); }

	void patch_cleared(const string& path)
		{ push_sig(sigc::bind(patch_cleared_slot, path)); }

	void object_renamed(const string& old_path, const string& new_path)
		{ push_sig(sigc::bind(object_renamed_slot, old_path, new_path)); }
	
	void disconnection(const string& src_port_path, const string& dst_port_path)
		{ push_sig(sigc::bind(disconnection_slot, src_port_path, dst_port_path)); }
	
	void metadata_update(const string& path, const string& key, const string& value)
		{ push_sig(sigc::bind(metadata_update_slot, path, key, value)); }

	void control_change(const string& port_path, float value)
		{ push_sig(sigc::bind(control_change_slot, port_path, value)); }

	void program_add(const string& path, uint32_t bank, uint32_t program, const string& name)
		{ push_sig(sigc::bind(program_add_slot, path, bank, program, name)); }
	
	void program_remove(const string& path, uint32_t bank, uint32_t program)
		{ push_sig(sigc::bind(program_remove_slot, path, bank, program)); }

	/** Process all queued events - Called from GTK thread to emit signals. */
	bool emit_signals();

private:
	void push_sig(Closure ev);
	
	Queue<Closure> _sigs;
	uint32_t       _num_plugins;

	sigc::slot<void>                                     bundle_begin_slot; 
	sigc::slot<void>                                     bundle_end_slot; 
	sigc::slot<void, uint32_t>                           num_plugins_slot; 
	sigc::slot<void, int32_t, bool, string>              response_slot; 
	sigc::slot<void, string>                             error_slot; 
	sigc::slot<void, string, string, string>             new_plugin_slot; 
	sigc::slot<void, string, uint32_t>                   new_patch_slot; 
	sigc::slot<void, string, string, string, bool, int>  new_node_slot; 
	sigc::slot<void, string, string, bool>               new_port_slot;
	sigc::slot<void, string, string>                     connection_slot;
	sigc::slot<void, string>                             patch_enabled_slot; 
	sigc::slot<void, string>                             patch_disabled_slot; 
	sigc::slot<void, string>                             patch_cleared_slot; 
	sigc::slot<void, string>                             object_destroyed_slot; 
	sigc::slot<void, string, string>                     object_renamed_slot; 
	sigc::slot<void, string, string>                     disconnection_slot; 
	sigc::slot<void, string, string, string>             metadata_update_slot; 
	sigc::slot<void, string, float>                      control_change_slot; 
	sigc::slot<void, string, uint32_t, uint32_t, string> program_add_slot; 
	sigc::slot<void, string, uint32_t, uint32_t>         program_remove_slot; 
};


} // namespace Client
} // namespace Ingen

#endif
