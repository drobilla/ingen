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
	SigClientInterface() : _enabled(true) {}

	// Signal parameters match up directly with ClientInterface calls
	
	sigc::signal<void, int32_t>                            signal_response_ok;
	sigc::signal<void, int32_t, string>                    signal_response_error;
	sigc::signal<void>                                     signal_bundle_begin; 
	sigc::signal<void>                                     signal_bundle_end; 
	sigc::signal<void, string>                             signal_error; 
	sigc::signal<void, uint32_t>                           signal_num_plugins; 
	sigc::signal<void, string, string, string, string>     signal_new_plugin; 
	sigc::signal<void, string, uint32_t>                   signal_new_patch; 
	sigc::signal<void, string, string, bool, uint32_t>     signal_new_node; 
	sigc::signal<void, string, string, bool>               signal_new_port; 
	sigc::signal<void, string, bool>                       signal_polyphonic; 
	sigc::signal<void, string>                             signal_patch_enabled; 
	sigc::signal<void, string>                             signal_patch_disabled; 
	sigc::signal<void, string, uint32_t>                   signal_patch_polyphony; 
	sigc::signal<void, string>                             signal_patch_cleared; 
	sigc::signal<void, string, string>                     signal_object_renamed; 
	sigc::signal<void, string>                             signal_object_destroyed; 
	sigc::signal<void, string, string>                     signal_connection; 
	sigc::signal<void, string, string>                     signal_disconnection; 
	sigc::signal<void, string, string, Raul::Atom>         signal_variable_change; 
	sigc::signal<void, string, float>                      signal_control_change; 
	sigc::signal<void, string>                             signal_port_activity; 
	sigc::signal<void, string, uint32_t, uint32_t, string> signal_program_add; 
	sigc::signal<void, string, uint32_t, uint32_t>         signal_program_remove; 
	
	/** Fire pending signals.  Only does anything on derived classes (that may queue) */
	virtual bool emit_signals() { return false; }

protected:
	
	bool _enabled;

	// ClientInterface hooks that fire the above signals
	
	void enable()  { _enabled = true; }
	void disable() { _enabled = false ; }

	void bundle_begin() {}
	void bundle_end()   {}
	
	void transfer_begin() {}
	void transfer_end()   {}

	void num_plugins(uint32_t num) { signal_num_plugins.emit(num); }

	void response_ok(int32_t id)
		{ if (_enabled) signal_response_ok.emit(id); }

	void response_error(int32_t id, const string& msg)
		{ if (_enabled) signal_response_error.emit(id, msg); }
	
	void error(const string& msg)
		{ if (_enabled) signal_error.emit(msg); }
	
	void new_plugin(const string& uri, const string& type_uri, const string& symbol, const string& name)
		{ if (_enabled) signal_new_plugin.emit(uri, type_uri, symbol, name); }
	
	void new_patch(const string& path, uint32_t poly)
		{ if (_enabled) signal_new_patch.emit(path, poly); }
	
	void new_node(const string& plugin_uri, const string& node_path, bool poly, uint32_t num_ports)
		{ if (_enabled) signal_new_node.emit(plugin_uri, node_path, poly, num_ports); }
	
	void new_port(const string& path, const string& data_type, bool is_output)
		{ if (_enabled) signal_new_port.emit(path, data_type, is_output); }
	
	void polyphonic(const string& path, bool polyphonic)
		{ if (_enabled) signal_polyphonic.emit(path, polyphonic); }

	void connection(const string& src_port_path, const string& dst_port_path)
		{ if (_enabled) signal_connection.emit(src_port_path, dst_port_path); }

	void object_destroyed(const string& path)
		{ if (_enabled) signal_object_destroyed.emit(path); }
	
	void patch_enabled(const string& path)
		{ if (_enabled) signal_patch_enabled.emit(path); }
	
	void patch_disabled(const string& path)
		{ if (_enabled) signal_patch_disabled.emit(path); }
	
	void patch_polyphony(const string& path, uint32_t poly)
		{ if (_enabled) signal_patch_polyphony.emit(path, poly); }

	void patch_cleared(const string& path)
		{ if (_enabled) signal_patch_cleared.emit(path); }

	void object_renamed(const string& old_path, const string& new_path)
		{ if (_enabled) signal_object_renamed.emit(old_path, new_path); }
	
	void disconnection(const string& src_port_path, const string& dst_port_path)
		{ if (_enabled) signal_disconnection.emit(src_port_path, dst_port_path); }
	
	void variable_change(const string& path, const string& key, const Raul::Atom& value)
		{ if (_enabled) signal_variable_change.emit(path, key, value); }

	void control_change(const string& port_path, float value)
		{ if (_enabled) signal_control_change.emit(port_path, value); }
	
	void port_activity(const string& port_path)
		{ if (_enabled) signal_port_activity.emit(port_path); }

	void program_add(const string& path, uint32_t bank, uint32_t program, const string& name)
		{ if (_enabled) signal_program_add.emit(path, bank, program, name); }
	
	void program_remove(const string& path, uint32_t bank, uint32_t program)
		{ if (_enabled) signal_program_remove.emit(path, bank, program); }
};


} // namespace Client
} // namespace Ingen

#endif
