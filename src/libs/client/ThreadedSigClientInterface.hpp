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

#ifndef THREADEDSIGCLIENTINTERFACE_H
#define THREADEDSIGCLIENTINTERFACE_H

#include <inttypes.h>
#include <string>
#include <sigc++/sigc++.h>
#include "interface/ClientInterface.hpp"
#include "SigClientInterface.hpp"
#include <raul/SRSWQueue.hpp>
#include <raul/Atom.hpp>
using std::string;

/** Returns nothing and takes no parameters (because they have all been bound) */
typedef sigc::slot<void> Closure;

namespace Ingen {
namespace Shared { class EngineInterface; }
namespace Client {


/** A LibSigC++ signal emitting interface for clients to use.
 *
 * This emits signals (possibly) in a different thread than the ClientInterface
 * functions are called.  It must be explicitly driven with the emit_signals()
 * function, which fires all enqueued signals up until the present.  You can
 * use this in a GTK idle callback for receiving thread safe engine signals.
 */
class ThreadedSigClientInterface : public SigClientInterface
{
public:
	ThreadedSigClientInterface(uint32_t queue_size)
	: _sigs(queue_size)
	, response_ok_slot(signal_response_ok.make_slot())
	, response_error_slot(signal_response_error.make_slot())
	, error_slot(signal_error.make_slot())
	, new_plugin_slot(signal_new_plugin.make_slot())
	, new_patch_slot(signal_new_patch.make_slot())
	, new_node_slot(signal_new_node.make_slot())
	, new_port_slot(signal_new_port.make_slot())
	, polyphonic_slot(signal_polyphonic.make_slot())
	, connection_slot(signal_connection.make_slot())
	, patch_enabled_slot(signal_patch_enabled.make_slot())
	, patch_disabled_slot(signal_patch_disabled.make_slot())
	, patch_polyphony_slot(signal_patch_polyphony.make_slot())
	, patch_cleared_slot(signal_patch_cleared.make_slot())
	, object_destroyed_slot(signal_object_destroyed.make_slot())
	, object_renamed_slot(signal_object_renamed.make_slot())
	, disconnection_slot(signal_disconnection.make_slot())
	, variable_change_slot(signal_variable_change.make_slot())
	, control_change_slot(signal_control_change.make_slot())
	, port_activity_slot(signal_port_activity.make_slot())
	, program_add_slot(signal_program_add.make_slot())
	, program_remove_slot(signal_program_remove.make_slot())
	{}

    virtual void subscribe(Shared::EngineInterface* engine) { throw; }

	void bundle_begin()
		{ push_sig(bundle_begin_slot); }

	void bundle_end()
		{ push_sig(bundle_end_slot); }
	
	void transfer_begin() {}
	void transfer_end()   {}

	void num_plugins(uint32_t num) { _num_plugins = num; }

	void response_ok(int32_t id)
		{ push_sig(sigc::bind(response_ok_slot, id)); }
	
	void response_error(int32_t id, const string& msg)
		{ push_sig(sigc::bind(response_error_slot, id, msg)); }

	void error(const string& msg)
		{ push_sig(sigc::bind(error_slot, msg)); }
	
	void new_plugin(const string& uri, const string& type_uri, const string& symbol, const string& name)
		{ push_sig(sigc::bind(new_plugin_slot, uri, type_uri, symbol, name)); }
	
	void new_patch(const string& path, uint32_t poly)
		{ push_sig(sigc::bind(new_patch_slot, path, poly)); }
	
	void new_node(const string& plugin_uri, const string& node_path, bool is_polyphonic, uint32_t num_ports)
		{ push_sig(sigc::bind(new_node_slot, plugin_uri, node_path, is_polyphonic, num_ports)); }
	
	void new_port(const string& path, uint32_t index,  const string& data_type, bool is_output)
		{ push_sig(sigc::bind(new_port_slot, path, index, data_type, is_output)); }
	
	void polyphonic(const string& path, bool polyphonic)
		{ push_sig(sigc::bind(polyphonic_slot, path, polyphonic)); }

	void connect(const string& src_port_path, const string& dst_port_path)
		{ push_sig(sigc::bind(connection_slot, src_port_path, dst_port_path)); }

	void object_destroyed(const string& path)
		{ push_sig(sigc::bind(object_destroyed_slot, path)); }
	
	void patch_enabled(const string& path)
		{ push_sig(sigc::bind(patch_enabled_slot, path)); }
	
	void patch_disabled(const string& path)
		{ push_sig(sigc::bind(patch_disabled_slot, path)); }
	
	void patch_polyphony(const string& path, uint32_t poly)
		{ push_sig(sigc::bind(patch_polyphony_slot, path, poly)); }

	void patch_cleared(const string& path)
		{ push_sig(sigc::bind(patch_cleared_slot, path)); }

	void object_renamed(const string& old_path, const string& new_path)
		{ push_sig(sigc::bind(object_renamed_slot, old_path, new_path)); }
	
	void disconnect(const string& src_port_path, const string& dst_port_path)
		{ push_sig(sigc::bind(disconnection_slot, src_port_path, dst_port_path)); }
	
	void set_variable(const string& path, const string& key, const Raul::Atom& value)
		{ push_sig(sigc::bind(variable_change_slot, path, key, value)); }

	void control_change(const string& port_path, float value)
		{ push_sig(sigc::bind(control_change_slot, port_path, value)); }
	
	void port_activity(const string& port_path)
		{ push_sig(sigc::bind(port_activity_slot, port_path)); }

	void program_add(const string& path, uint32_t bank, uint32_t program, const string& name)
		{ push_sig(sigc::bind(program_add_slot, path, bank, program, name)); }
	
	void program_remove(const string& path, uint32_t bank, uint32_t program)
		{ push_sig(sigc::bind(program_remove_slot, path, bank, program)); }

	/** Process all queued events - Called from GTK thread to emit signals. */
	bool emit_signals();

private:
	void push_sig(Closure ev);
	
	Raul::SRSWQueue<Closure> _sigs;
	uint32_t                 _num_plugins;

	sigc::slot<void>                                     bundle_begin_slot; 
	sigc::slot<void>                                     bundle_end_slot; 
	sigc::slot<void, uint32_t>                           num_plugins_slot; 
	sigc::slot<void, int32_t>                            response_ok_slot; 
	sigc::slot<void, int32_t, string>                    response_error_slot; 
	sigc::slot<void, string>                             error_slot; 
	sigc::slot<void, string, string, string, string>     new_plugin_slot; 
	sigc::slot<void, string, uint32_t>                   new_patch_slot; 
	sigc::slot<void, string, string, bool, int>          new_node_slot; 
	sigc::slot<void, string, uint32_t, string, bool>     new_port_slot;
	sigc::slot<void, string, bool>                       polyphonic_slot;
	sigc::slot<void, string, string>                     connection_slot;
	sigc::slot<void, string>                             patch_enabled_slot; 
	sigc::slot<void, string>                             patch_disabled_slot; 
	sigc::slot<void, string, uint32_t>                   patch_polyphony_slot; 
	sigc::slot<void, string>                             patch_cleared_slot; 
	sigc::slot<void, string>                             object_destroyed_slot; 
	sigc::slot<void, string, string>                     object_renamed_slot; 
	sigc::slot<void, string, string>                     disconnection_slot; 
	sigc::slot<void, string, string, Raul::Atom>         variable_change_slot; 
	sigc::slot<void, string, float>                      control_change_slot; 
	sigc::slot<void, string>                             port_activity_slot; 
	sigc::slot<void, string, uint32_t, uint32_t, string> program_add_slot; 
	sigc::slot<void, string, uint32_t, uint32_t>         program_remove_slot; 
};


} // namespace Client
} // namespace Ingen

#endif
