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

#ifndef INGEN_ENGINE_ENGINE_PORT_HPP
#define INGEN_ENGINE_ENGINE_PORT_HPP

#include <string>

#include "raul/Deletable.hpp"
#include "raul/Noncopyable.hpp"

#include "DuplexPort.hpp"

namespace Raul { class Path; }

namespace Ingen {
namespace Server {

class DuplexPort;
class ProcessContext;

/** Representation of a "system" port (a port outside Ingen).
 *
 * This is the class through which the rest of the engine manages everything
 * related to driver ports.  Derived classes are expected to have a pointer to
 * their driver (to be able to perform the operation necessary).
 *
 * \ingroup engine
 */
class EnginePort : public Raul::Noncopyable, public Raul::Deletable {
public:
	EnginePort() : _patch_port(NULL), _buffer(NULL) {}
	virtual ~EnginePort() {}

	/** Set the name of the system port according to new path */
	virtual void move(const Raul::Path& path) = 0;

	/** Create system port */
	virtual void create()  = 0;

	/** Destroy system port */
	virtual void destroy() = 0;

	void* buffer() const        { return _buffer; }
	void  set_buffer(void* buf) { _buffer = buf; }

	bool        is_input()   const { return _patch_port->is_input(); }
	DuplexPort* patch_port() const { return _patch_port; }

protected:
	explicit EnginePort(DuplexPort* port) : _patch_port(port) {}

	DuplexPort* _patch_port;
	void*       _buffer;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_ENGINE_PORT_HPP
