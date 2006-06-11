/* This file is part of Om.  Copyright (C) 2005 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

using std::list; using std::string; using std::map;

namespace LibOmClient {

class ConnectionModel;
	

/** Client's model of a patch.
 *
 * \ingroup libomclient
 */
class PatchModel : public NodeModel
{
public:
	PatchModel(const string& patch_path, uint poly)
	: NodeModel(patch_path),
	  m_enabled(false),
	  m_poly(poly)
	{}
	
	const NodeModelMap&           nodes()       const { return m_nodes; }
	const list<ConnectionModel*>& connections() const { return m_connections; }
	
	virtual void set_path(const Path& path);
	
	NodeModel*       get_node(const string& node_name);
	void             add_node(CountedPtr<NodeModel> nm);
	void             remove_node(const string& name);

	void             rename_node(const Path& old_path, const Path& new_path);
	void             rename_node_port(const Path& old_path, const Path& new_path);
	ConnectionModel* get_connection(const string& src_port_path, const string& dst_port_path);
	void             add_connection(ConnectionModel* cm);
	void             remove_connection(const string& src_port_path, const string& dst_port_path);
		
	virtual void clear();
	
	size_t        poly() const               { return m_poly; }
	void          poly(size_t p)             { m_poly = p; }
	const string& filename() const           { return m_filename; }
	void          filename(const string& f)  { m_filename = f; }
	bool          enabled() const            { return m_enabled; }
	void          enabled(bool b)            { m_enabled = b; }
	bool          polyphonic() const;
	
	// Signals
	sigc::signal<void, CountedPtr<NodeModel> > new_node_sig; 

private:
	// Prevent copies (undefined)
	PatchModel(const PatchModel& copy);
	PatchModel& operator=(const PatchModel& copy);
	
	NodeModelMap             m_nodes;
	list<ConnectionModel*>   m_connections;
	string                   m_filename;
	bool                     m_enabled;
	size_t                   m_poly;
};

typedef map<string, PatchModel*> PatchModelMap;


} // namespace LibOmClient

#endif // PATCHMODEL_H
