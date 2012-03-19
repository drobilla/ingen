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

#include <iostream>
#include <utility>

#include "ingen/shared/AtomReader.hpp"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"
#include "raul/log.hpp"

namespace Ingen {
namespace Shared {

AtomReader::AtomReader(LV2URIMap& map, URIs& uris, Forge& forge, Interface& iface)
	: _map(map)
	, _uris(uris)
	, _forge(forge)
	, _iface(iface)
{
}

void
AtomReader::write(const LV2_Atom* msg)
{
	if (msg->type != _uris.atom_Blank) {
		Raul::warn << "Unknown message type " << msg->type << std::endl;
		return;
	}

	const LV2_Atom_Object* obj     = (const LV2_Atom_Object*)msg;
	const LV2_Atom*        subject = NULL;

	lv2_object_get(obj, (LV2_URID)_uris.patch_subject, &subject, NULL);
	const char* subject_uri = subject ? (const char*)LV2_ATOM_BODY(subject) : NULL;
	if (obj->body.otype == _uris.patch_Get) {
		Raul::warn << "ATOM GET " << subject_uri << std::endl;
		_iface.set_response_id(1);
		_iface.get(subject_uri);
	} else if (obj->body.otype == _uris.patch_Put) {
		Raul::warn << "PUT" << std::endl;
		const LV2_Atom_Object* body = NULL;
		lv2_object_get(obj, (LV2_URID)_uris.patch_body, &body, 0);
		if (!body) {
			Raul::warn << "NO BODY" << std::endl;
			return;
		}

		Ingen::Resource::Properties props;
		LV2_OBJECT_FOREACH(body, i) {
			LV2_Atom_Property_Body* p = lv2_object_iter_get(i);
			props.insert(std::make_pair(_map.unmap_uri(p->key),
			                            _forge.alloc(p->value.size,
			                                         p->value.type,
			                                         LV2_ATOM_BODY(&p->value))));
		}

		if (subject_uri) {
			_iface.put(subject_uri, props);
		} else {
			Raul::warn << "Put message has no subject, ignored" << std::endl;
		}

	} else {
		Raul::warn << "HANDLE MESSAGE TYPE " << obj->body.otype << std::endl;
	}
}

} // namespace Shared
} // namespace Ingen
