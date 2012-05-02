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

#include <utility>

#include "ingen/shared/AtomReader.hpp"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"
#include "raul/Path.hpp"
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
AtomReader::get_atom(const LV2_Atom* in, Raul::Atom& out)
{
	if (in) {
		if (in->type == _uris.atom_URID) {
			const LV2_Atom_URID* urid = (const LV2_Atom_URID*)in;
			out = _forge.alloc_uri(_map.unmap_uri(urid->body));
		} else {
			out = _forge.alloc(in->size, in->type, LV2_ATOM_BODY(in));
		}
	}
}

void
AtomReader::get_props(const LV2_Atom_Object*       obj,
                      Ingen::Resource::Properties& props)
{
	if (obj->body.otype) {
		props.insert(
			std::make_pair(_uris.rdf_type,
			               _forge.alloc_uri(_map.unmap_uri(obj->body.otype))));
	}
	LV2_ATOM_OBJECT_FOREACH(obj, p) {
		Raul::Atom val;
		get_atom(&p->value, val);
		props.insert(std::make_pair(_map.unmap_uri(p->key), val));
	}
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

	lv2_atom_object_get(obj, (LV2_URID)_uris.patch_subject, &subject, NULL);
	const char* subject_uri = NULL;
	if (subject && subject->type == _uris.atom_URI) {
		subject_uri = (const char*)LV2_ATOM_BODY(subject);
	} else if (subject && subject->type == _uris.atom_URID) {
		subject_uri = _map.unmap_uri(((LV2_Atom_URID*)subject)->body);
	}

	if (obj->body.otype == _uris.patch_Get) {
		_iface.set_response_id(obj->body.id);
		_iface.get(subject_uri);
	} else if (obj->body.otype == _uris.patch_Delete) {
		if (subject_uri) {
			_iface.del(subject_uri);
			return;
		}

		const LV2_Atom_Object* body = NULL;
		lv2_atom_object_get(obj, (LV2_URID)_uris.patch_body, &body, 0);
		if (body && body->body.otype == _uris.ingen_Edge) {
			const LV2_Atom* tail = NULL;
			const LV2_Atom* head = NULL;
			lv2_atom_object_get(body,
			                    (LV2_URID)_uris.ingen_tail, &tail,
			                    (LV2_URID)_uris.ingen_head, &head,
			                    NULL);

			Raul::Atom tail_atom;
			Raul::Atom head_atom;
			get_atom(tail, tail_atom);
			get_atom(head, head_atom);
			if (tail_atom.is_valid() && head_atom.is_valid()) {
				_iface.disconnect(Raul::Path(tail_atom.get_uri()),
				                  Raul::Path(head_atom.get_uri()));
			} else {
				Raul::warn << "Delete of unknown object." << std::endl;
			}
		}
	} else if (obj->body.otype == _uris.patch_Put) {
		const LV2_Atom_Object* body = NULL;
		lv2_atom_object_get(obj, (LV2_URID)_uris.patch_body, &body, 0);
		if (!body) {
			Raul::warn << "Put message has no body" << std::endl;
			return;
		} else if (!subject_uri) {
			Raul::warn << "Put message has no subject" << std::endl;
			return;
		}

		if (body->body.otype == _uris.ingen_Edge) {
			LV2_Atom* tail = NULL;
			LV2_Atom* head = NULL;
			lv2_atom_object_get(body,
			                    (LV2_URID)_uris.ingen_tail, &tail,
			                    (LV2_URID)_uris.ingen_head, &head,
			                    NULL);
			if (!tail || !head) {
				Raul::warn << "Edge has no tail or head" << std::endl;
				return;
			}

			Raul::Atom tail_atom;
			Raul::Atom head_atom;
			get_atom(tail, tail_atom);
			get_atom(head, head_atom);
			_iface.connect(Raul::Path(tail_atom.get_uri()),
			               Raul::Path(head_atom.get_uri()));
		} else {
			Ingen::Resource::Properties props;
			get_props(body, props);
			_iface.set_response_id(obj->body.id);
			_iface.put(subject_uri, props);
		}
	} else if (obj->body.otype == _uris.patch_Set) {
		const LV2_Atom_Object* body = NULL;
		lv2_atom_object_get(obj, (LV2_URID)_uris.patch_body, &body, 0);
		if (!body) {
			Raul::warn << "Set message has no body" << std::endl;
			return;
		} else if (!subject_uri) {
			Raul::warn << "Set message has no subject" << std::endl;
			return;
		}

		LV2_ATOM_OBJECT_FOREACH(body, p) {
			Raul::Atom val;
			get_atom(&p->value, val);
			_iface.set_property(subject_uri, _map.unmap_uri(p->key), val);
		}
	} else if (obj->body.otype == _uris.patch_Patch) {
		if (!subject_uri) {
			Raul::warn << "Put message has no subject" << std::endl;
			return;
		}

		const LV2_Atom_Object* remove = NULL;
		lv2_atom_object_get(obj, (LV2_URID)_uris.patch_remove, &remove, 0);
		if (!remove) {
			Raul::warn << "Patch message has no remove" << std::endl;
			return;
		}

		const LV2_Atom_Object* add = NULL;
		lv2_atom_object_get(obj, (LV2_URID)_uris.patch_add, &add, 0);
		if (!add) {
			Raul::warn << "Patch message has no add" << std::endl;
			return;
		}

		Ingen::Resource::Properties add_props;
		get_props(remove, add_props);

		Ingen::Resource::Properties remove_props;
		get_props(remove, remove_props);

		_iface.delta(subject_uri, remove_props, add_props);
	} else {
		Raul::warn << "Unknown object type <"
		           << _map.unmap_uri(obj->body.otype)
		           << ">" << std::endl;
	}
}

} // namespace Shared
} // namespace Ingen
