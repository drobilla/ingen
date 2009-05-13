/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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

#ifndef DRIVER_H
#define DRIVER_H

#include <string>
#include <boost/utility.hpp>
#include "raul/Deletable.hpp"
#include "interface/DataType.hpp"
#include "DuplexPort.hpp"

namespace Raul { class Path; }

namespace Ingen {

class DuplexPort;


/** Representation of a "system" (eg outside Ingen) port.
 *
 * This is the class through which the rest of the engine manages everything
 * related to driver ports.  Derived classes are expected to have a pointer to
 * their driver (to be able to perform the operation necessary).
 *
 * \ingroup engine
 */
class DriverPort : boost::noncopyable, public Raul::Deletable {
public:
	virtual ~DriverPort() {}

	/** Set the name of the system port */
	virtual void set_name(const std::string& name) = 0;

	virtual void create()  = 0;
	virtual void destroy() = 0;

	bool        is_input()   const { return _patch_port->is_input(); }
	DuplexPort* patch_port() const { return _patch_port; }

protected:
	/** is_input from the perspective outside of ingen */
	DriverPort(DuplexPort* port) : _patch_port(port) {}

	DuplexPort* _patch_port;
};


/** Driver abstract base class.
 *
 * A Driver is, from the perspective of GraphObjects (nodes, patches, ports) an
 * interface for managing system ports.  An implementation of Driver basically
 * needs to manage DriverPorts, and handle writing/reading data to/from them.
 *
 * \ingroup engine
 */
class Driver : boost::noncopyable
{
public:
	Driver(Shared::DataType type)
		: _type(type)
	{}

	virtual ~Driver() {}

	virtual void activate()   = 0;
	virtual void deactivate() = 0;

	virtual bool is_activated() const = 0;

	/** Create a port ready to be inserted with add_input (non realtime).
	 *
	 * May return NULL if the Driver can not drive the port for some reason.
	 */
	virtual DriverPort* create_port(DuplexPort* patch_port) = 0;

	virtual DriverPort* driver_port(const Raul::Path& path) = 0;

	virtual void add_port(DriverPort* port) = 0;

	virtual Raul::List<DriverPort*>::Node* remove_port(const Raul::Path& path) = 0;

protected:
	Shared::DataType _type;
};


} // namespace Ingen

#endif // DRIVER_H
