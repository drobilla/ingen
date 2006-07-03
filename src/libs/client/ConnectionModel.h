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
#include "util/CountedPtr.h"
#include "PortModel.h"
#include <cassert>
using std::string;
using Om::Path;

namespace LibOmClient {


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
	ConnectionModel(CountedPtr<PortModel> src, CountedPtr<PortModel> dst);

	CountedPtr<PortModel> src_port() const { return _src_port; }
	CountedPtr<PortModel> dst_port() const { return _dst_port; }

	void set_src_port(CountedPtr<PortModel> port) { _src_port = port; _src_port_path = port->path(); }
	void set_dst_port(CountedPtr<PortModel> port) { _dst_port = port; _dst_port_path = port->path(); }

	void src_port_path(const string& s) { _src_port_path = s; }
	void dst_port_path(const string& s) { _dst_port_path = s; }
	
	const Path& src_port_path() const;
	const Path& dst_port_path() const;
	const Path  patch_path()    const;
	
private:
	Path                  _src_port_path; ///< Only used if _src_port == NULL
	Path                  _dst_port_path; ///< Only used if _dst_port == NULL
	CountedPtr<PortModel> _src_port;
	CountedPtr<PortModel> _dst_port;
};


} // namespace LibOmClient

#endif // CONNECTIONMODEL_H
