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

#ifndef DIRECTSIGCLIENTINTERFACE_H
#define DIRECTSIGCLIENTINTERFACE_H

#include <inttypes.h>
#include <string>
#include <sigc++/sigc++.h>
#include "SigClientInterface.hpp"
using std::string;

namespace Ingen {
namespace Client {


/** A direct (nonthreaded) LibSigC++ signal emitting interface for clients to use.
 * 
 * The signals from SigClientInterface will be emitted in the same thread as the
 * ClientInterface functions are called.  <b>You can not set this directly as an
 * in-engine client interface and connect the signals to GTK</b>.
 *
 * For maximum performance of a monolithic single-client GUI app, it would be
 * nice if the post processing thread in the engine could actually be the GTK
 * thread, then you could use this directly and minimize queueing of events and
 * thread/scheduling overhead.
 *
 * sed would have the copyright to this code if it was a legal person.
 */
class DirectSigClientInterface : virtual public SigClientInterface
{
public:
	DirectSigClientInterface();

private:

	// ClientInterface function implementations to drive SigClientInterface signals
	
	virtual void bundle_begin()
	{ bundle_begin_sig.emit(); }

	virtual void bundle_end()
	{ bundle_end_sig.emit(); }

	virtual void error(const string& msg)
	{ error_sig.emit(msg); }
	
	virtual void num_plugins(uint32_t num)
	{ num_plugins_sig.emit(num); }

	virtual void new_plugin(const string& type, const string& uri, const string& name)
	{ new_plugin_sig.emit(type, uri, name); }
	
	virtual void new_patch(const string& path, uint32_t poly)
	{ new_patch_sig.emit(path, poly); }
	
	virtual void new_node(const string& plugin_type, const string& plugin_uri, const string& node_path, bool is_polyphonic, uint32_t num_ports)
	{ new_node_sig.emit(plugin_type, plugin_uri, node_path, is_polyphonic, num_ports); }
	
	virtual void new_port(const string& path, const string& data_type, bool is_output)
	{ new_port_sig.emit(path, data_type, is_output); }
	
	virtual void polyphonic(const string& path, bool polyphonic)
	{ polyphonic_sig.emit(path, polyphonic); }
	
	virtual void patch_enabled(const string& path)
	{ patch_enabled_sig.emit(path); }
	
	virtual void patch_disabled(const string& path)
	{ patch_disabled_sig.emit(path); }
	
	virtual void patch_cleared(const string& path)
	{ patch_cleared_sig.emit(path); }
	
	virtual void object_renamed(const string& old_path, const string& new_path)
	{ object_renamed_sig.emit(old_path, new_path); }
	
	virtual void object_destroyed(const string& path)
	{ object_destroyed_sig.emit(path); }
	
	virtual void connection(const string& src_port_path, const string& dst_port_path)
	{ connection_sig.emit(src_port_path, dst_port_path); }
	
	virtual void disconnection(const string& src_port_path, const string& dst_port_path)
	{ disconnection_sig.emit(src_port_path, dst_port_path); }
	
	virtual void metadata_update(const string& subject_path, const string& predicate, const string& value)
	{ metadata_update_sig.emit(subject_path, predicate, value); }
	
	virtual void control_change(const string& port_path, float value)
	{ control_change_sig.emit(port_path, value); }
	
	virtual void program_add(const string& node_path, uint32_t bank, uint32_t program, const string& program_name)
	{ program_add_sig.emit(node_path, bank, program, program_name); }
	
	virtual void program_remove(const string& node_path, uint32_t bank, uint32_t program)
	{ program_remove_sig.emit(node_path, bank, program); }
};


} // namespace Client
} // namespace Ingen

#endif
