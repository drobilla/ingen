/* This file is part of Ingen.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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

#ifndef INGEN_ENGINE_DRIVER_HPP
#define INGEN_ENGINE_DRIVER_HPP

#include <string>
#include <boost/utility.hpp>
#include "raul/Deletable.hpp"
#include "interface/PortType.hpp"
#include "interface/EventType.hpp"
#include "DuplexPort.hpp"

namespace Raul { class Path; }

namespace Ingen {

class DuplexPort;
class ProcessContext;


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

	/** Set the name of the system port according to new path */
	virtual void move(const Raul::Path& path) = 0;

	/** Create system port */
	virtual void create()  = 0;

	/** Destroy system port */
	virtual void destroy() = 0;

	bool        is_input()   const { return _patch_port->is_input(); }
	DuplexPort* patch_port() const { return _patch_port; }

protected:
	explicit DriverPort(DuplexPort* port) : _patch_port(port) {}

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
class Driver : boost::noncopyable {
public:
	virtual ~Driver() {}

	/** Activate driver (begin processing graph and events). */
	virtual void activate() {}

	/** Deactivate driver (stop processing graph and events). */
	virtual void deactivate() {}

	/** Return true iff driver is activated (processing graph and events). */
	virtual bool is_activated() const { return true; }

	/** Create a port ready to be inserted with add_input (non realtime).
	 * May return NULL if the Driver can not create the port for some reason.
	 */
	virtual DriverPort* create_port(DuplexPort* patch_port) = 0;

	/** Return the DriverPort for a particular path, iff one exists. */
	virtual DriverPort* driver_port(const Raul::Path& path) = 0;

	/** Add a system visible port (e.g. a port on the root patch). */
	virtual void add_port(DriverPort* port) = 0;

	/** Remove a system visible port. */
	virtual Raul::Deletable* remove_port(const Raul::Path& path, Ingen::DriverPort** port=NULL) = 0;

	/** Return true iff this driver supports the given type of I/O */
	virtual bool supports(Shared::PortType port_type, Shared::EventType event_type) = 0;

	virtual void       set_root_patch(PatchImpl* patch) = 0;
	virtual PatchImpl* root_patch()                     = 0;

	/** Return the audio buffer size in frames (i.e. the maximum length of a process cycle) */
	virtual SampleCount block_length() const = 0;

	/** Return the sample rate in Hz */
	virtual SampleCount sample_rate() const = 0;

	/** Return the current frame time (running counter) */
	virtual SampleCount frame_time()  const = 0;

	virtual bool is_realtime() const = 0;

	virtual ProcessContext& context() = 0;
};


} // namespace Ingen

#endif // INGEN_ENGINE_DRIVER_HPP
