/* This file is part of Ingen.
 * Copyright 2012 David Robillard <http://drobilla.net>
 *
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef INGEN_SHARED_ATOM_WRITER_HPP
#define INGEN_SHARED_ATOM_WRITER_HPP

#include "ingen/Interface.hpp"
#include "ingen/shared/LV2URIMap.hpp"
#include "ingen/shared/URIs.hpp"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include "serd/serd.h"

namespace Ingen {
namespace Shared {

class AtomSink;

/** An Interface that writes LV2 atoms. */
class AtomWriter : public Interface
{
public:
	AtomWriter(LV2URIMap& map, URIs& uris, AtomSink& sink);
	~AtomWriter() {}

	Raul::URI uri() const { return "http://drobilla.net/ns/ingen#AtomWriter"; }

	void bundle_begin();

	void bundle_end();

	void put(const Raul::URI&            uri,
	         const Resource::Properties& properties,
	         Resource::Graph             ctx = Resource::DEFAULT);

	void delta(const Raul::URI&            uri,
	           const Resource::Properties& remove,
	           const Resource::Properties& add);

	void move(const Raul::Path& old_path,
	          const Raul::Path& new_path);

	void del(const Raul::URI& uri);

	void connect(const Raul::Path& src_port_path,
	             const Raul::Path& dst_port_path);

	void disconnect(const Raul::URI& src,
	                const Raul::URI& dst);

	void disconnect_all(const Raul::Path& parent_patch_path,
	                    const Raul::Path& path);

	void set_property(const Raul::URI&  subject,
	                  const Raul::URI&  predicate,
	                  const Raul::Atom& value);

	void set_response_id(int32_t id);

	void get(const Raul::URI& uri);

	void response(int32_t id, Status status);

	void error(const std::string& msg);

private:
	void    finish_msg();
	int32_t next_id();

	LV2URIMap&     _map;
	URIs&          _uris;
	AtomSink&      _sink;
	SerdChunk      _out;
	LV2_Atom_Forge _forge;
	int32_t        _id;
};

} // namespace Shared
} // namespace Ingen

#endif // INGEN_SHARED_ATOM_WRITER_HPP

