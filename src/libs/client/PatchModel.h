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

#ifndef PATCHMODEL_H
#define PATCHMODEL_H

#include <cassert>
#include <list>
#include <string>
#include <map>
#include <sigc++/sigc++.h>
#include "NodeModel.h"
#include "raul/SharedPtr.h"
#include "ConnectionModel.h"

using std::list; using std::string; using std::map;

namespace Ingen {
namespace Client {

class Store;


/** Client's model of a patch.
 *
 * \ingroup IngenClient
 */
class PatchModel : public NodeModel
{
public:
	const NodeModelMap&   nodes()       const { return _nodes; }
	const ConnectionList& connections() const { return _connections; }
	
	SharedPtr<ConnectionModel> get_connection(const string& src_port_path, const string& dst_port_path) const;
	SharedPtr<NodeModel>       get_node(const string& node_name) const;
	
	size_t        poly()       const { return _poly; }
	const string& filename()   const { return _filename; }
	bool          enabled()    const { return _enabled; }
	bool          polyphonic() const;
	
	// Signals
	sigc::signal<void, SharedPtr<NodeModel> >       new_node_sig; 
	sigc::signal<void, SharedPtr<NodeModel> >       removed_node_sig; 
	sigc::signal<void, SharedPtr<ConnectionModel> > new_connection_sig; 
	sigc::signal<void, const Path&, const Path& >    removed_connection_sig;
	sigc::signal<void>                               enabled_sig;
	sigc::signal<void>                               disabled_sig;

private:
	friend class Store;

	PatchModel(const Path& patch_path, size_t internal_poly)
	: NodeModel("ingen:patch", patch_path, false), // FIXME
	  _enabled(false),
	  _poly(internal_poly)
	{
	}
	
	void filename(const string& f) { _filename = f; }
	void poly(size_t p)            { _poly = p; }
	void enable();
	void disable();
	void clear();
	void set_path(const Path& path);
	void add_node(SharedPtr<NodeModel> nm);
	void remove_node(SharedPtr<NodeModel> nm);
	void add_child(SharedPtr<ObjectModel> c);
	void remove_child(SharedPtr<ObjectModel> c);
	
	void add_connection(SharedPtr<ConnectionModel> cm);
	void remove_connection(const string& src_port_path, const string& dst_port_path);
	
	void rename_node(const Path& old_path, const Path& new_path);
	void rename_node_port(const Path& old_path, const Path& new_path);

	NodeModelMap   _nodes;
	ConnectionList _connections;
	string         _filename;
	bool           _enabled;
	size_t         _poly;
};

typedef map<string, SharedPtr<PatchModel> > PatchModelMap;


} // namespace Client
} // namespace Ingen

#endif // PATCHMODEL_H
