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

#ifndef DRIVER_H
#define DRIVER_H

#include <string>
#include <boost/utility.hpp>
#include <raul/Path.hpp>
#include "DataType.hpp"

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
class DriverPort : boost::noncopyable {
public:
	virtual ~DriverPort() {}

	/** Set the name of the system port */
	virtual void set_name(const std::string& name) = 0;
	
	bool is_input() { return _is_input; }

protected:
	/** is_input from the perspective outside of ingen */
	DriverPort(bool is_input) : _is_input(is_input) {}

	bool _is_input;
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
	Driver(DataType type)
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
	
	virtual void        add_port(DriverPort* port)          = 0;
	virtual DriverPort* remove_port(const Raul::Path& path) = 0;

protected:
	DataType _type;
};


} // namespace Ingen

#endif // DRIVER_H
