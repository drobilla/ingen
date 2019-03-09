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

#include "ingen/Configuration.hpp"
#include "ingen/SocketReader.hpp"
#include "ingen/SocketWriter.hpp"
#include "ingen/StreamWriter.hpp"
#include "ingen/Tee.hpp"
#include "ingen/World.hpp"
#include "raul/Socket.hpp"

namespace ingen {
namespace server {

/** The server side of an Ingen socket connection. */
class SocketServer
{
public:
	SocketServer(World&             world,
	             server::Engine&    engine,
	             SPtr<Raul::Socket> sock)
		: _engine(engine)
		, _sink(world.conf().option("dump").get<int32_t>()
		        ? SPtr<Interface>(
			        new Tee({SPtr<Interface>(new EventWriter(engine)),
					         SPtr<Interface>(new StreamWriter(world.rdf_world(),
					                                          world.uri_map(),
					                                          world.uris(),
					                                          URI("ingen:/engine"),
					                                          stderr,
					                                          ColorContext::Color::CYAN))}))
		        : SPtr<Interface>(new EventWriter(engine)))
		, _reader(new SocketReader(world, *_sink.get(), sock))
		, _writer(new SocketWriter(world.rdf_world(),
		                           world.uri_map(),
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
	server::Engine&    _engine;
	SPtr<Interface>    _sink;
	SPtr<SocketReader> _reader;
	SPtr<SocketWriter> _writer;
};

}  // namespace ingen
}  // namespace Socket

#endif  // INGEN_SERVER_SOCKET_SERVER_HPP
