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

#include "ingen/TurtleWriter.hpp"

#include "ingen/URIMap.hpp"
#include "lv2/atom/atom.h"

namespace ingen {

TurtleWriter::TurtleWriter(serd::World& world,
                           URIMap&      map,
                           URIs&        uris,
                           const URI&   uri)
    : AtomWriter(world, map, uris, *this)
    , _map(map)
    , _streamer(world,
                map.urid_map_feature()->urid_map,
                map.urid_unmap_feature()->urid_unmap)
    , _base(serd::make_uri("ingen:/")) // Make relative URIs like bundle paths
    , _env(_base)
    , _writer(world,
              serd::Syntax::Turtle,
              {},
              _env,
              [&](const char* str, size_t len) { return text_sink(str, len); })
    , _uri(uri)
    , _wrote_prefixes(false)
{
	// Set namespace prefixes
	_env.set_prefix("atom",  "http://lv2plug.in/ns/ext/atom#");
	_env.set_prefix("doap",  "http://usefulinc.com/ns/doap#");
	_env.set_prefix("ingen", INGEN_NS);
	_env.set_prefix("lv2",   "http://lv2plug.in/ns/lv2core#");
	_env.set_prefix("midi",  "http://lv2plug.in/ns/ext/midi#");
	_env.set_prefix("owl",   "http://www.w3.org/2002/07/owl#");
	_env.set_prefix("patch", "http://lv2plug.in/ns/ext/patch#");
	_env.set_prefix("rdf",   "http://www.w3.org/1999/02/22-rdf-syntax-ns#");
	_env.set_prefix("rdfs",  "http://www.w3.org/2000/01/rdf-schema#");
	_env.set_prefix("xsd",   "http://www.w3.org/2001/XMLSchema#");
}

TurtleWriter::~TurtleWriter()
{
	_writer.finish();
}

bool
TurtleWriter::write(const LV2_Atom* msg, int32_t)
{
	if (!_wrote_prefixes) {
		// Write namespace prefixes once to reduce traffic
		_env.write_prefixes(_writer.sink());
		_writer.finish();
		_wrote_prefixes = true;
	}

	_streamer.write(_writer.sink(), *msg);
	_writer.finish();
	return true;
}

} // namespace ingen
