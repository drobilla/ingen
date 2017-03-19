/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

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

#include "raul/Deletable.hpp"
#include "raul/Noncopyable.hpp"

#include <boost/intrusive/slist.hpp>

#include "DuplexPort.hpp"

namespace Ingen {
namespace Server {

/** A "system" port (e.g. a Jack port, an external port on Ingen).
 *
 * @ingroup engine
 */
class EnginePort : public Raul::Noncopyable
                 , public Raul::Deletable
                 , public boost::intrusive::slist_base_hook<>
{
public:
	explicit EnginePort(DuplexPort* port)
		: _graph_port(port)
		, _buffer(NULL)
		, _handle(NULL)
		, _driver_index(0)
	{}

	void set_buffer(void* buf)            { _buffer = buf; }
	void set_handle(void* buf)            { _handle = buf; }
	void set_driver_index(uint32_t index) { _driver_index = index; }

	void*       buffer()       const { return _buffer; }
	void*       handle()       const { return _handle; }
	uint32_t    driver_index() const { return _driver_index; }
	DuplexPort* graph_port()   const { return _graph_port; }
	bool        is_input()     const { return _graph_port->is_input(); }

protected:
	DuplexPort* _graph_port;
	void*       _buffer;
	void*       _handle;
	uint32_t    _driver_index;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_ENGINE_PORT_HPP
