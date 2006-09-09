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

#ifndef MODELCLIENTINTERFACE_H
#define MODELCLIENTINTERFACE_H

#include <string>
#include <memory>
using std::string; using std::auto_ptr;
#include "interface/ClientInterface.h"

namespace Ingen {
namespace Client {

class PatchModel;
class NodeModel;
class ConnectionModel;
class PortModel;
class PluginModel;


/** A client interface that creates Model objects to represent the engine's state.
 *
 * This calls it's own methods with the models as parameters; clients can inherit
 * this and implement a class with a similar interface to ClientInterface except
 * with model classes passed where appropriate instead of primitives.
 *
 * \ingroup IngenClient
 */
class ModelClientInterface : virtual public Ingen::Shared::ClientInterface
{
public:
	ModelClientInterface(Ingen::Shared::ClientInterface& extend)
	: Ingen::Shared::ClientInterface(extend)
	{}
	
	virtual ~ModelClientInterface() {}

	// FIXME: make these auto_ptr's
	
	virtual void new_plugin_model(PluginModel* pi);
	virtual void new_patch_model(PatchModel* pm);
	virtual void new_node_model(NodeModel* nm);
	virtual void new_port_model(PortModel* port_info);
	virtual void connection_model(ConnectionModel* cm);

	// ClientInterface functions to drive the above:
	
	virtual void new_plugin(string type,
	                        string uri,
	                        string name);
	
	virtual void new_patch(string path, uint32_t poly);
	
	virtual void new_node(string   plugin_type,
	                      string   plugin_uri,
	                      string   node_path,
	                      bool     is_polyphonic,
	                      uint32_t num_ports);
	
	virtual void new_port(string path,
	                      string data_type,
	                      bool   is_output);
	
	virtual void connection(string src_port_path,
	                        string dst_port_path);

protected:
	ModelClientInterface() {}
};


} // namespace Client
} // namespace Ingen

#endif // MODELCLIENTINTERFACE_H
