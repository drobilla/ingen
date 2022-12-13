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

#ifndef INGEN_SERVER_SOCKET_SERVER_HPP
#define INGEN_SERVER_SOCKET_SERVER_HPP

#include "EventWriter.hpp"

#include "Engine.hpp"

#include "ingen/Atom.hpp"
#include "ingen/ColorContext.hpp"
#include "ingen/Configuration.hpp"
#include "ingen/Interface.hpp"
#include "ingen/SocketReader.hpp"
#include "ingen/SocketWriter.hpp"
#include "ingen/StreamWriter.hpp"
#include "ingen/Tee.hpp"
#include "ingen/URI.hpp"
#include "ingen/World.hpp"
#include "raul/Socket.hpp"

#include <cstdint>
#include <cstdio>
#include <memory>

namespace ingen::server {

/** The server side of an Ingen socket connection. */
class SocketServer
{
public:
	SocketServer(World&                               world,
	             server::Engine&                      engine,
	             const std::shared_ptr<raul::Socket>& sock)
		: _engine(engine)
		, _sink(world.conf().option("dump").get<int32_t>()
		        ? std::shared_ptr<Interface>(
			        new Tee({std::shared_ptr<Interface>(new EventWriter(engine)),
					         std::shared_ptr<Interface>(new StreamWriter(world.uri_map(),
					                                          world.uris(),
					                                          URI("ingen:/engine"),
					                                          stderr,
					                                          ColorContext::Color::CYAN))}))
		        : std::shared_ptr<Interface>(new EventWriter(engine)))
		, _reader(new SocketReader(world, *_sink, sock))
		, _writer(new SocketWriter(world.uri_map(),
		                           world.uris(),
		                           URI(sock->uri()),
		                           sock))
	{
		_sink->set_respondee(_writer);
		engine.register_client(_writer);
	}

	~SocketServer() {
		if (_writer) {
			_engine.unregister_client(_writer);
		}
	}

protected:
	void on_hangup() {
		_engine.unregister_client(_writer);
		_writer.reset();
	}

private:
	server::Engine&               _engine;
	std::shared_ptr<Interface>    _sink;
	std::shared_ptr<SocketReader> _reader;
	std::shared_ptr<SocketWriter> _writer;
};

} // namespace ingen::server

#endif // INGEN_SERVER_SOCKET_SERVER_HPP
