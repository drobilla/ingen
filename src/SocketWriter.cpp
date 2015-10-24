/*
  This file is part of Ingen.
  Copyright 2012-2015 David Robillard <http://drobilla.net/>

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

#include "ingen/SocketWriter.hpp"
#include "ingen/URIMap.hpp"

#ifndef MSG_NOSIGNAL
#    define MSG_NOSIGNAL 0
#endif

#define USTR(s) ((const uint8_t*)(s))

namespace Ingen {

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

static SerdStatus
write_prefix(void* handle, const SerdNode* name, const SerdNode* uri)
{
	SocketWriter* writer = (SocketWriter*)handle;
	serd_writer_set_prefix(writer->writer(), name, uri);
	return SERD_SUCCESS;
}

SocketWriter::SocketWriter(URIMap&            map,
                           URIs&              uris,
                           const Raul::URI&   uri,
                           SPtr<Raul::Socket> sock)
	: AtomWriter(map, uris, *this)
	, _map(map)
	, _sratom(sratom_new(&map.urid_map_feature()->urid_map))
	, _uri(uri)
	, _socket(sock)
{
	// Use <ingen:/> as base URI, so relative URIs are like bundle paths
	_base = serd_node_from_string(SERD_URI, (const uint8_t*)"ingen:/");

	serd_uri_parse(_base.buf, &_base_uri);

	// Set up serialisation environment
	_env = serd_env_new(&_base);
	serd_env_set_prefix_from_strings(_env, USTR("atom"),  USTR("http://lv2plug.in/ns/ext/atom#"));
	serd_env_set_prefix_from_strings(_env, USTR("patch"), USTR("http://lv2plug.in/ns/ext/patch#"));
	serd_env_set_prefix_from_strings(_env, USTR("doap"),  USTR("http://usefulinc.com/ns/doap#"));
	serd_env_set_prefix_from_strings(_env, USTR("ingen"), USTR(INGEN_NS));
	serd_env_set_prefix_from_strings(_env, USTR("lv2"),   USTR("http://lv2plug.in/ns/lv2core#"));
	serd_env_set_prefix_from_strings(_env, USTR("midi"),  USTR("http://lv2plug.in/ns/ext/midi#"));
	serd_env_set_prefix_from_strings(_env, USTR("owl"),   USTR("http://www.w3.org/2002/07/owl#"));
	serd_env_set_prefix_from_strings(_env, USTR("rdf"),   USTR("http://www.w3.org/1999/02/22-rdf-syntax-ns#"));
	serd_env_set_prefix_from_strings(_env, USTR("rdfs"),  USTR("http://www.w3.org/2000/01/rdf-schema#"));
	serd_env_set_prefix_from_strings(_env, USTR("xsd"),   USTR("http://www.w3.org/2001/XMLSchema#"));

	// Make a Turtle writer that writes directly to the socket
	_writer = serd_writer_new(
		SERD_TURTLE,
		(SerdStyle)(SERD_STYLE_RESOLVED|SERD_STYLE_ABBREVIATED|SERD_STYLE_CURIED),
		_env,
		&_base_uri,
		socket_sink,
		this);

	// Write namespace prefixes to reduce traffic
	serd_env_foreach(_env, write_prefix, this);

	// Configure sratom to write directly to the writer (and thus the socket)
	sratom_set_sink(_sratom,
	                (const char*)_base.buf,
	                (SerdStatementSink)serd_writer_write_statement,
	                (SerdEndSink)serd_writer_end_anon,
	                _writer);
}

SocketWriter::~SocketWriter()
{
	sratom_free(_sratom);
	serd_writer_free(_writer);
	serd_env_free(_env);
}

bool
SocketWriter::write(const LV2_Atom* msg)
{
	sratom_write(_sratom, &_map.urid_unmap_feature()->urid_unmap, 0,
	             NULL, NULL, msg->type, msg->size, LV2_ATOM_BODY_CONST(msg));
	serd_writer_finish(_writer);
	return true;
}

void
SocketWriter::bundle_end()
{
	AtomWriter::bundle_end();

	// Send a NULL byte to indicate end of bundle
	const char end[] = { 0 };
	send(fd(), end, 1, MSG_NOSIGNAL);
}

} // namespace Ingen
