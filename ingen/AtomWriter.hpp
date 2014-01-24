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

#ifndef INGEN_ATOMWRITER_HPP
#define INGEN_ATOMWRITER_HPP

#include <string>

#include "ingen/Interface.hpp"
#include "ingen/URIs.hpp"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include "serd/serd.h"

namespace Ingen {

class AtomSink;
class URIMap;

/** An Interface that writes LV2 atoms to an AtomSink. */
class AtomWriter : public Interface
{
public:
	AtomWriter(URIMap& map, URIs& uris, AtomSink& sink);
	~AtomWriter() {}

	Raul::URI uri() const {
		return Raul::URI("ingen:/clients/atom_writer");
	}

	void bundle_begin();

	void bundle_end();

	void put(const Raul::URI&            uri,
	         const Resource::Properties& properties,
	         Resource::Graph             ctx = Resource::Graph::DEFAULT);

	void delta(const Raul::URI&            uri,
	           const Resource::Properties& remove,
	           const Resource::Properties& add);

	void move(const Raul::Path& old_path,
	          const Raul::Path& new_path);

	void del(const Raul::URI& uri);

	void connect(const Raul::Path& tail,
	             const Raul::Path& head);

	void disconnect(const Raul::Path& tail,
	                const Raul::Path& head);

	void disconnect_all(const Raul::Path& graph,
	                    const Raul::Path& path);

	void set_property(const Raul::URI& subject,
	                  const Raul::URI& predicate,
	                  const Atom&      value);

	void set_response_id(int32_t id);

	void get(const Raul::URI& uri);

	void response(int32_t id, Status status, const std::string& subject);

	void error(const std::string& msg);

private:
	void forge_uri(const Raul::URI& uri);
	void forge_properties(const Resource::Properties& properties);
	void forge_arc(const Raul::Path& tail, const Raul::Path& head);
	void forge_request(LV2_Atom_Forge_Frame* frame, LV2_URID type);

	void    finish_msg();
	int32_t next_id();

	URIMap&        _map;
	URIs&          _uris;
	AtomSink&      _sink;
	SerdChunk      _out;
	LV2_Atom_Forge _forge;
	int32_t        _id;
};

} // namespace Ingen

#endif // INGEN_ATOMWRITER_HPP
