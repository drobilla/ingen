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

#ifndef DRIVER_H
#define DRIVER_H

#include <string>
#include <boost/utility.hpp>

namespace Ingen {

template <typename T> class DuplexPort;


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

	/** Add this port to driver at the beginning of a process cycle (realtime safe) */
	virtual void add_to_driver() = 0;
	
	/** Remove this port at the beginning of a process cycle (realtime safe) */
	virtual void remove_from_driver() = 0;

	/** Set the name of the system port */
	virtual void set_name(const std::string& name) = 0;
	
protected:
	DriverPort() {}
};


/** Driver abstract base class.
 *
 * A Driver is, from the perspective of GraphObjects (nodes, patches, ports) an
 * interface for managing system ports.  An implementation of Driver basically
 * needs to manage DriverPorts, and handle writing/reading data to/from them.
 *
 * The template parameter T is the type of data this driver manages (ie the
 * data type of the bridge ports it will handle).
 *
 * \ingroup engine
 */
template <typename T>
class Driver : boost::noncopyable
{
public:
	virtual ~Driver() {}

	virtual void activate()   = 0;
	virtual void deactivate() = 0;
	
	virtual bool is_activated() const = 0;

	/** Create a port ready to be inserted with add_input (non realtime).
	 *
	 * May return NULL if the Driver can not drive the port for some reason.
	 */
	virtual DriverPort* create_port(DuplexPort<T>* patch_port) = 0;
};


#if 0
/** Dummy audio driver.
 *
 * Not abstract, all functions are dummies.  One of these will be allocated and
 * "used" if no working AUDIO driver is loaded.  (Doing it this way as opposed to
 * just making Driver have dummy functions makes sure any existing Driver
 * derived class actually implements the required functions).
 *
 * \ingroup engine
 */
class DummyDriver : public Driver
{
public:
	~DummyDriver() {}

	void activate()   {}
	void deactivate() {}
	
	void enable()  {}
	void disable() {}
	
	DriverPort* create_port(TypedPort<Sample>* patch_port) { return NULL; }
};
#endif


} // namespace Ingen

#endif // DRIVER_H
