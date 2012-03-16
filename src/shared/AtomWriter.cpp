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

#include "raul/Path.hpp"

#include "AtomWriter.hpp"

namespace Ingen {
namespace Shared {

AtomWriter::AtomWriter(LV2URIMap& map, URIs& uris)
	: _map(map)
	, _uris(uris)
	, _id(-1)
{
	lv2_atom_forge_init(&_forge, &map.urid_map_feature()->urid_map);
}

int32_t
AtomWriter::next_id()
{
	if (_id == -1) {
		return 0;
	} else {
		return ++_id;
	}
}

void
AtomWriter::bundle_begin()
{
}

void
AtomWriter::bundle_end()
{
}

void
AtomWriter::put(const Raul::URI&            uri,
                const Resource::Properties& properties,
                Resource::Graph             ctx)
{
}

void
AtomWriter::delta(const Raul::URI&            uri,
                  const Resource::Properties& remove,
                  const Resource::Properties& add)
{
}

void
AtomWriter::move(const Raul::Path& old_path,
                 const Raul::Path& new_path)
{
}

void
AtomWriter::del(const Raul::URI& uri)
{
}

void
AtomWriter::connect(const Raul::Path& src,
                    const Raul::Path& dst)
{
	LV2_Atom_Forge_Frame msg;
	lv2_atom_forge_blank(&_forge, &msg, next_id(), _uris.patch_Put);
	lv2_atom_forge_property_head(&_forge, _uris.patch_body, 0);

	LV2_Atom_Forge_Frame body;
	lv2_atom_forge_blank(&_forge, &body, 0, _uris.ingen_Connection);
	lv2_atom_forge_property_head(&_forge, _uris.ingen_source, 0);
	lv2_atom_forge_string(&_forge, src.c_str(), src.length());
	lv2_atom_forge_property_head(&_forge, _uris.ingen_destination, 0);
	lv2_atom_forge_string(&_forge, dst.c_str(), dst.length());
	lv2_atom_forge_pop(&_forge, &body);

	lv2_atom_forge_pop(&_forge, &msg);
}

void
AtomWriter::disconnect(const Raul::URI& src,
                       const Raul::URI& dst)
{
}

void
AtomWriter::disconnect_all(const Raul::Path& parent_patch_path,
                           const Raul::Path& path)
{
}

void
AtomWriter::set_property(const Raul::URI&  subject,
                         const Raul::URI&  predicate,
                         const Raul::Atom& value)
{
}

void
AtomWriter::set_response_id(int32_t id)
{
}

void
AtomWriter::get(const Raul::URI& uri)
{
	LV2_Atom_Forge_Frame msg;
	lv2_atom_forge_blank(&_forge, &msg, next_id(), _uris.patch_Get);
	lv2_atom_forge_property_head(&_forge, _uris.patch_subject, 0);
	lv2_atom_forge_uri(&_forge, uri.c_str(), uri.length());
	lv2_atom_forge_pop(&_forge, &msg);
}

void
AtomWriter::response(int32_t id, Status status)
{
	LV2_Atom_Forge_Frame msg;
	lv2_atom_forge_blank(&_forge, &msg, next_id(), _uris.patch_Response);
	lv2_atom_forge_property_head(&_forge, _uris.patch_request, 0);
	lv2_atom_forge_int32(&_forge, id);
	lv2_atom_forge_pop(&_forge, &msg);
}

void
AtomWriter::error(const std::string& msg)
{
}

} // namespace Shared
} // namespace Ingen
