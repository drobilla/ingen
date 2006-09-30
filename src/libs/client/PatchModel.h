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

#ifndef PATCHMODEL_H
#define PATCHMODEL_H

#include <cassert>
#include <list>
#include <string>
#include <map>
#include <sigc++/sigc++.h>
#include "NodeModel.h"
#include "util/CountedPtr.h"
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
	const NodeModelMap&   nodes()       const { return m_nodes; }
	const ConnectionList& connections() const { return m_connections; }
	
	CountedPtr<ConnectionModel> get_connection(const string& src_port_path, const string& dst_port_path) const;
	CountedPtr<NodeModel>       get_node(const string& node_name) const;
	
	size_t        poly()       const { return m_poly; }
	const string& filename()   const { return m_filename; }
	bool          enabled()    const { return m_enabled; }
	bool          polyphonic() const;
	
	// Signals
	sigc::signal<void, CountedPtr<NodeModel> >       new_node_sig; 
	sigc::signal<void, CountedPtr<NodeModel> >       removed_node_sig; 
	sigc::signal<void, CountedPtr<ConnectionModel> > new_connection_sig; 
	sigc::signal<void, const Path&, const Path& >    removed_connection_sig;
	sigc::signal<void>                               enabled_sig;
	sigc::signal<void>                               disabled_sig;

private:
	friend class Store;

	PatchModel(const Path& patch_path, size_t internal_poly)
	: NodeModel("ingen:patch", patch_path, false), // FIXME
	  m_enabled(false),
	  m_poly(internal_poly)
	{
	}
	
	void filename(const string& f) { m_filename = f; }
	void poly(size_t p)            { m_poly = p; }
	void enable();
	void disable();
	void clear();
	void set_path(const Path& path);
	void add_node(CountedPtr<NodeModel> nm);
	void remove_node(CountedPtr<NodeModel> nm);
	void add_child(CountedPtr<ObjectModel> c);
	void remove_child(CountedPtr<ObjectModel> c);
	
	void add_connection(CountedPtr<ConnectionModel> cm);
	void remove_connection(const string& src_port_path, const string& dst_port_path);
	
	void rename_node(const Path& old_path, const Path& new_path);
	void rename_node_port(const Path& old_path, const Path& new_path);

	// Prevent copies (undefined)
	PatchModel(const PatchModel& copy);
	PatchModel& operator=(const PatchModel& copy);
	
	NodeModelMap   m_nodes;
	ConnectionList m_connections;
	string         m_filename;
	bool           m_enabled;
	size_t         m_poly;
};

typedef map<string, CountedPtr<PatchModel> > PatchModelMap;


} // namespace Client
} // namespace Ingen

#endif // PATCHMODEL_H
