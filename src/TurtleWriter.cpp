/*
  This file is part of Ingen.
  Copyright 2012-2016 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <ingen/TurtleWriter.hpp>

#include <ingen/AtomWriter.hpp>
#include <ingen/URI.hpp>
#include <ingen/URIMap.hpp>
#include <ingen/ingen.h>
#include <lv2/atom/atom.h>
#include <serd/serd.h>
#include <sratom/sratom.h>

#include <utility>

#define USTR(s) reinterpret_cast<const uint8_t*>(s)

namespace ingen {
namespace {

size_t
c_text_sink(const void* buf, size_t len, void* stream)
{
	auto* writer = static_cast<TurtleWriter*>(stream);
	return writer->text_sink(buf, len);
}

SerdStatus
write_prefix(void* handle, const SerdNode* name, const SerdNode* uri)
{
	serd_writer_set_prefix(static_cast<SerdWriter*>(handle), name, uri);
	return SERD_SUCCESS;
}

} // namespace

TurtleWriter::TurtleWriter(URIMap& map, URIs& uris, URI uri)
	: AtomWriter{map, uris, *this}
    , _map{map}
    , _sratom{sratom_new(&map.urid_map())}
    , _base{serd_node_from_string(SERD_URI, USTR("ingen:/"))}
    , _env{serd_env_new(&_base)}
    , _uri{std::move(uri)}
{
	// Use <ingen:/> as base URI, so relative URIs are like bundle paths

	serd_uri_parse(USTR("ingen:/"), &_base_uri);

	// Set up serialisation environment
	serd_env_set_prefix_from_strings(_env, USTR("atom"),  USTR("http://lv2plug.in/ns/ext/atom#"));
	serd_env_set_prefix_from_strings(_env, USTR("doap"),  USTR("http://usefulinc.com/ns/doap#"));
	serd_env_set_prefix_from_strings(_env, USTR("ingen"), USTR(INGEN_NS));
	serd_env_set_prefix_from_strings(_env, USTR("lv2"),   USTR("http://lv2plug.in/ns/lv2core#"));
	serd_env_set_prefix_from_strings(_env, USTR("midi"),  USTR("http://lv2plug.in/ns/ext/midi#"));
	serd_env_set_prefix_from_strings(_env, USTR("owl"),   USTR("http://www.w3.org/2002/07/owl#"));
	serd_env_set_prefix_from_strings(_env, USTR("patch"), USTR("http://lv2plug.in/ns/ext/patch#"));
	serd_env_set_prefix_from_strings(_env, USTR("rdf"),   USTR("http://www.w3.org/1999/02/22-rdf-syntax-ns#"));
	serd_env_set_prefix_from_strings(_env, USTR("rdfs"),  USTR("http://www.w3.org/2000/01/rdf-schema#"));
	serd_env_set_prefix_from_strings(_env, USTR("xsd"),   USTR("http://www.w3.org/2001/XMLSchema#"));

	// Make a Turtle writer that writes to text_sink
	// NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
	_writer = serd_writer_new(
		SERD_TURTLE,
		static_cast<SerdStyle>(SERD_STYLE_RESOLVED|SERD_STYLE_ABBREVIATED|SERD_STYLE_CURIED),
		_env,
		&_base_uri,
		c_text_sink,
		this);

	// Configure sratom to write directly to the writer (and thus text_sink)
	sratom_set_sink(_sratom,
	                reinterpret_cast<const char*>(_base.buf),
	                reinterpret_cast<SerdStatementSink>(serd_writer_write_statement),
	                reinterpret_cast<SerdEndSink>(serd_writer_end_anon),
	                _writer);
}

TurtleWriter::~TurtleWriter()
{
	sratom_free(_sratom);
	serd_writer_free(_writer);
	serd_env_free(_env);
}

bool
TurtleWriter::write(const LV2_Atom* msg, int32_t)
{
	if (!_wrote_prefixes) {
		// Write namespace prefixes once to reduce traffic
		serd_env_foreach(_env, write_prefix, _writer);
		_wrote_prefixes = true;
	}

	sratom_write(_sratom, &_map.urid_unmap(), 0,
	             nullptr, nullptr, msg->type, msg->size, LV2_ATOM_BODY_CONST(msg));
	serd_writer_finish(_writer);
	return true;
}

} // namespace ingen
