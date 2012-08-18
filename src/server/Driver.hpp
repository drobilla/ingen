/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_ENGINE_DRIVER_HPP
#define INGEN_ENGINE_DRIVER_HPP

#include <boost/intrusive/list.hpp>

#include "raul/Noncopyable.hpp"

#include "DuplexPort.hpp"
#include "EnginePort.hpp"

namespace Raul { class Path; }

namespace Ingen {
namespace Server {

class DuplexPort;
class EnginePort;

/** Engine driver base class.
 *
 * A Driver is, from the perspective of GraphObjects (blocks, patches, ports)
 * an interface for managing system ports.  An implementation of Driver
 * basically needs to manage EnginePorts, and handle writing/reading data
 * to/from them.
 *
 * \ingroup engine
 */
class Driver : public Raul::Noncopyable {
public:
	virtual ~Driver() {}

	/** Activate driver (begin processing graph and events). */
	virtual void activate() {}

	/** Deactivate driver (stop processing graph and events). */
	virtual void deactivate() {}

	/** Create a port ready to be inserted with add_input (non realtime).
	 * May return NULL if the Driver can not create the port for some reason.
	 */
	virtual EnginePort* create_port(DuplexPort* patch_port) = 0;

	/** Find a system port by path. */
	virtual EnginePort* get_port(const Raul::Path& path) = 0;

	/** Add a system visible port (e.g. a port on the root patch). */
	virtual void add_port(ProcessContext& context, EnginePort* port) = 0;

	/** Remove a system visible port.
	 *
	 * This removes the port from the driver in the process thread but does not
	 * destroy the port.  To actually remove the system port, unregister_port()
	 * must be called later in another thread.
	 */
	virtual void remove_port(ProcessContext& context, EnginePort* port) = 0;

	/** Register a system visible port. */
	virtual void register_port(EnginePort& port) = 0;

	/** Register a system visible port. */
	virtual void unregister_port(EnginePort& port) = 0;

	/** Rename a system visible port. */
	virtual void rename_port(const Raul::Path& old_path,
	                         const Raul::Path& new_path) = 0;

	/** Return the audio buffer size in frames */
	virtual SampleCount block_length() const = 0;

	/** Return the sample rate in Hz */
	virtual SampleCount sample_rate() const = 0;

	/** Return the current frame time (running counter) */
	virtual SampleCount frame_time()  const = 0;

	/** Return true iff the driver is running in real-time mode */
	virtual bool is_realtime() const = 0;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_DRIVER_HPP
