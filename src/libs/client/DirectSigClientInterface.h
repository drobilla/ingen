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

#ifndef DIRECTSIGCLIENTINTERFACE_H
#define DIRECTSIGCLIENTINTERFACE_H

#include <inttypes.h>
#include <string>
#include <sigc++/sigc++.h>
#include "SigClientInterface.h"
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
	{ emit_bundle_begin(); }

	virtual void bundle_end()
	{ emit_bundle_end(); }

	virtual void error(const string& msg)
	{ emit_error(msg); }
	
	virtual void num_plugins(uint32_t num)
	{ emit_num_plugins(num); }

	virtual void new_plugin(const string& type, const string& uri, const string& name)
	{ emit_new_plugin(type, uri, name); }
	
	virtual void new_patch(const string& path, uint32_t poly)
	{ emit_new_patch(path, poly); }
	
	virtual void new_node(const string& plugin_type, const string& plugin_uri, const string& node_path, bool is_polyphonic, uint32_t num_ports)
	{ emit_new_node(plugin_type, plugin_uri, node_path, is_polyphonic, num_ports); }
	
	virtual void new_port(const string& path, const string& data_type, bool is_output)
	{ emit_new_port(path, data_type, is_output); }
	
	virtual void patch_enabled(const string& path)
	{ emit_patch_enabled(path); }
	
	virtual void patch_disabled(const string& path)
	{ emit_patch_disabled(path); }
	
	virtual void patch_cleared(const string& path)
	{ emit_patch_cleared(path); }
	
	virtual void object_renamed(const string& old_path, const string& new_path)
	{ emit_object_renamed(old_path, new_path); }
	
	virtual void object_destroyed(const string& path)
	{ emit_object_destroyed(path); }
	
	virtual void connection(const string& src_port_path, const string& dst_port_path)
	{ emit_connection(src_port_path, dst_port_path); }
	
	virtual void disconnection(const string& src_port_path, const string& dst_port_path)
	{ emit_disconnection(src_port_path, dst_port_path); }
	
	virtual void metadata_update(const string& subject_path, const string& predicate, const string& value)
	{ emit_metadata_update(subject_path, predicate, value); }
	
	virtual void control_change(const string& port_path, float value)
	{ emit_control_change(port_path, value); }
	
	virtual void program_add(const string& node_path, uint32_t bank, uint32_t program, const string& program_name)
	{ emit_program_add(node_path, bank, program, program_name); }
	
	virtual void program_remove(const string& node_path, uint32_t bank, uint32_t program)
	{ emit_program_remove(node_path, bank, program); }
};


} // namespace Client
} // namespace Ingen

#endif
