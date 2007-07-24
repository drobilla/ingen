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
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef NODEMODEL_H
#define NODEMODEL_H

#include <cstdlib>
#include <map>
#include <iostream>
#include <string>
#include <sigc++/sigc++.h>
#include "ObjectModel.hpp"
#include "PortModel.hpp"
#include <raul/Path.hpp>
#include <raul/SharedPtr.hpp>
#include "PluginModel.hpp"

using std::string; using std::map; using std::find;
using std::cout; using std::cerr; using std::endl;

namespace Ingen {
namespace Client {

class PluginModel;
class Store;

	
/** Node model class, used by the client to store engine's state.
 *
 * \ingroup IngenClient
 */
class NodeModel : public ObjectModel
{
public:
	virtual ~NodeModel();

	SharedPtr<PortModel> get_port(const string& port_name) const;
	
	const map<int, map<int, string> >& get_programs() const { return _banks; }
	const string&                      plugin_uri()   const { return _plugin_uri; }
	SharedPtr<PluginModel>             plugin()       const { return _plugin; }
	int                                num_ports()    const { return _ports.size(); }
	const PortModelList&               ports()        const { return _ports; }
	virtual bool                       polyphonic()   const { return _polyphonic; }
	
	// Signals
	sigc::signal<void, SharedPtr<PortModel> > new_port_sig; 
	sigc::signal<void, SharedPtr<PortModel> > removed_port_sig; 
	
protected:
	friend class Store;
	
	NodeModel(const string& plugin_uri, const Path& path, bool polyphonic);
	NodeModel(SharedPtr<PluginModel> plugin, const Path& path, bool polyphonic);

	NodeModel(const Path& path);
	void add_child(SharedPtr<ObjectModel> c);
	void remove_child(SharedPtr<ObjectModel> c);
	void add_port(SharedPtr<PortModel> pm);
	void remove_port(SharedPtr<PortModel> pm);
	void remove_port(const Path& port_path);
	void add_program(int bank, int program, const string& name);
	void remove_program(int bank, int program);

	virtual void clear();
	
	friend class PatchModel;
	void set_path(const Path& p);

	bool                        _polyphonic;
	PortModelList               _ports;      ///< List of ports (not a map to preserve order)
	string                      _plugin_uri; ///< Plugin URI (if PluginModel is unknown)
	SharedPtr<PluginModel>      _plugin;     ///< The plugin this node is an instance of
	map<int, map<int, string> > _banks;      ///< DSSI banks
};


typedef map<string, SharedPtr<NodeModel> > NodeModelMap;


} // namespace Client
} // namespace Ingen

#endif // NODEMODEL_H
