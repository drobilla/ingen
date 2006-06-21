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
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */


#ifndef CONNECTIONMODEL_H
#define CONNECTIONMODEL_H

#include <string>
#include "util/Path.h"
#include <cassert>
using std::string;
using Om::Path;

namespace LibOmClient {

class PortModel;


/** Class to represent a port->port connection in the engine.
 *
 * This can either have pointers to the connection ports' models, or just
 * paths as strings.  LibOmClient passes just strings (by necessity), but
 * clients can set the pointers then they don't have to worry about port
 * renaming, as the connections will always return the port's path, even
 * if it changes.
 *
 * \ingroup libomclient
 */
class ConnectionModel
{
public:
	ConnectionModel(const Path& src_port, const Path& dst_port);

	PortModel* src_port() const { return m_src_port; }
	PortModel* dst_port() const { return m_dst_port; }

	void set_src_port(PortModel* port) { m_src_port = port; m_src_port_path = ""; }
	void set_dst_port(PortModel* port) { m_dst_port = port; m_dst_port_path = ""; }

	void src_port_path(const string& s) { m_src_port_path = s; }
	void dst_port_path(const string& s) { m_dst_port_path = s; }
	
	const Path& src_port_path() const;
	const Path& dst_port_path() const;
	const Path  patch_path()    const;
	
private:
	Path m_src_port_path;  ///< Only used if m_src_port == NULL
	Path m_dst_port_path;  ///< Only used if m_dst_port == NULL
	PortModel* m_src_port;
	PortModel* m_dst_port;
};


} // namespace LibOmClient

#endif // CONNECTIONMODEL_H
