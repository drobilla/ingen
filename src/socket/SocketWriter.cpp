/*
  This file is part of Ingen.
  Copyright 2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "ingen/shared/URIMap.hpp"

#include "SocketWriter.hpp"

namespace Ingen {
namespace Socket {

static size_t
socket_sink(const void* buf, size_t len, void* stream)
{
	SocketWriter* writer = (SocketWriter*)stream;
	ssize_t       ret    = send(writer->fd(), buf, len, MSG_NOSIGNAL);
	if (ret < 0) {
		return 0;
	}
	return ret;
}

SocketWriter::SocketWriter(Shared::URIMap&   map,
                           Shared::URIs&     uris,
                           const Raul::URI&  uri,
                           SharedPtr<Socket> sock)
	: AtomWriter(map, uris, *this)
	, _map(map)
	, _sratom(sratom_new(&map.urid_map_feature()->urid_map))
	, _uri(uri)
	, _socket(sock)
{
	// Use <path:> as base URI so e.g. </foo/bar> will be a path
	_base = serd_node_from_string(SERD_URI, (const uint8_t*)"path:");

	serd_uri_parse(_base.buf, &_base_uri);

	_env    = serd_env_new(&_base);
	_writer = serd_writer_new(
		SERD_TURTLE,
		(SerdStyle)(SERD_STYLE_RESOLVED|SERD_STYLE_ABBREVIATED|SERD_STYLE_CURIED),
		_env,
		&_base_uri,
		socket_sink,
		this);

	sratom_set_sink(_sratom,
	                (const char*)_base.buf,
	                (SerdStatementSink)serd_writer_write_statement,
	                (SerdEndSink)serd_writer_end_anon,
	                _writer);
}

SocketWriter::~SocketWriter()
{
	sratom_free(_sratom);
}

void
SocketWriter::write(const LV2_Atom* msg)
{
	sratom_write(_sratom, &_map.urid_unmap_feature()->urid_unmap, 0,
	             NULL, NULL, msg->type, msg->size, LV2_ATOM_BODY(msg));
	serd_writer_finish(_writer);
}

} // namespace Socket
} // namespace Ingen
