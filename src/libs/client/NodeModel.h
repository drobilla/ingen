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

#ifndef NODEMODEL_H
#define NODEMODEL_H

#include <cstdlib>
#include <map>
#include <iostream>
#include <string>
#include <sigc++/sigc++.h>
#include "ObjectModel.h"
#include "PortModel.h"
#include "util/Path.h"
#include "util/CountedPtr.h"
#include "PluginModel.h"

using std::string; using std::map; using std::find;
using std::cout; using std::cerr; using std::endl;

namespace Ingen {
namespace Client {

class PluginModel;

	
/** Node model class, used by the client to store engine's state.
 *
 * \ingroup IngenClient
 */
class NodeModel : public ObjectModel
{
public:
	NodeModel(const string& plugin_uri, const Path& path);
	NodeModel(CountedPtr<PluginModel> plugin, const Path& path);
	virtual ~NodeModel();
	
	void add_child(CountedPtr<ObjectModel> c);

	CountedPtr<PortModel> get_port(const string& port_name);
	void       add_port(CountedPtr<PortModel> pm);
	void       remove_port(const string& port_path);
	
	virtual void clear();

	const map<int, map<int, string> >& get_programs() const     { return m_banks; }
	void add_program(int bank, int program, const string& name);
	void remove_program(int bank, int program);

	string plugin_uri() { return m_plugin_uri; }

	CountedPtr<PluginModel> plugin() const { return m_plugin; }
	//void               plugin(CountedPtr<PluginModel> p) { m_plugin = p; }
	
	virtual void set_path(const Path& p);
	
	int                   num_ports() const  { return m_ports.size(); }
	const PortModelList&  ports() const      { return m_ports; }
	virtual bool          polyphonic() const { return m_polyphonic; }
	void                  polyphonic(bool b) { m_polyphonic = b; }
	float                 x() const          { return m_x; }
	float                 y() const          { return m_y; }
	void                  x(float a);
	void                  y(float a);
	
	// Signals
	sigc::signal<void, CountedPtr<PortModel> > new_port_sig; 
	
protected:
	NodeModel(const Path& path);

	bool                        m_polyphonic;
	PortModelList               m_ports;  ///< List of ports (instead of map to preserve order)
	string                      m_plugin_uri; ///< Plugin URI (not redundant if PluginModel unknown
	CountedPtr<PluginModel>     m_plugin; ///< The plugin this node is an instance of
	float                       m_x;      ///< Just metadata, here as an optimization for GUI
	float                       m_y;      ///< Just metadata, here as an optimization for GUI
	map<int, map<int, string> > m_banks;  ///< DSSI banks

private:
	// Prevent copies (undefined)
	NodeModel(const NodeModel& copy);
	NodeModel& operator=(const NodeModel& copy);	
};


typedef map<string, CountedPtr<NodeModel> > NodeModelMap;


} // namespace Client
} // namespace Ingen

#endif // NODEMODEL_H
