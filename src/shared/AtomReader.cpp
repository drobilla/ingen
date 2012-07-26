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
#include "ingen/shared/URIMap.hpp"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"
#include "raul/Path.hpp"
#include "raul/log.hpp"

namespace Ingen {
namespace Shared {

AtomReader::AtomReader(URIMap& map, URIs& uris, Forge& forge, Interface& iface)
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
			const char* uri           = _map.unmap_uri(urid->body);
			if (uri) {
				out = _forge.alloc_uri(_map.unmap_uri(urid->body));
			} else {
				Raul::error(Raul::fmt("Unable to unmap URID %1%\n")
				            % urid->body);
			}
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

const char*
AtomReader::atom_to_uri(const LV2_Atom* atom)
{
	if (atom && atom->type == _uris.atom_URI) {
		return (const char*)LV2_ATOM_BODY(atom);
	} else if (atom && atom->type == _uris.atom_URID) {
		return _map.unmap_uri(((LV2_Atom_URID*)atom)->body);
	} else {
		return NULL;
	}
}

bool
AtomReader::is_message(URIs& uris, const LV2_Atom* msg)
{
	if (msg->type != uris.atom_Blank && msg->type != uris.atom_Resource) {
		return false;
	}

	const LV2_Atom_Object* obj = (const LV2_Atom_Object*)msg;
	return (obj->body.otype == uris.patch_Get ||
	        obj->body.otype == uris.patch_Delete ||
	        obj->body.otype == uris.patch_Put ||
	        obj->body.otype == uris.patch_Patch ||
	        obj->body.otype == uris.patch_Move ||
	        obj->body.otype == uris.patch_Response);
}

bool
AtomReader::write(const LV2_Atom* msg)
{
	if (msg->type != _uris.atom_Blank && msg->type != _uris.atom_Resource) {
		Raul::warn << (Raul::fmt("Unknown message type <%1%>\n")
		               % _map.unmap_uri(msg->type)).str();
		return false;
	}

	const LV2_Atom_Object* obj     = (const LV2_Atom_Object*)msg;
	const LV2_Atom*        subject = NULL;

	lv2_atom_object_get(obj, (LV2_URID)_uris.patch_subject, &subject, NULL);
	const char* subject_uri = atom_to_uri(subject);

	if (obj->body.otype == _uris.patch_Get) {
		_iface.set_response_id(obj->body.id);
		_iface.get(subject_uri);
	} else if (obj->body.otype == _uris.patch_Delete) {
		const LV2_Atom_Object* body = NULL;
		lv2_atom_object_get(obj, (LV2_URID)_uris.patch_body, &body, 0);

		if (subject_uri && !body) {
			_iface.del(subject_uri);
			return true;
		} else if (body && body->body.otype == _uris.ingen_Edge) {
			const LV2_Atom* tail       = NULL;
			const LV2_Atom* head       = NULL;
			const LV2_Atom* incidentTo = NULL;
			lv2_atom_object_get(body,
			                    (LV2_URID)_uris.ingen_tail, &tail,
			                    (LV2_URID)_uris.ingen_head, &head,
			                    (LV2_URID)_uris.ingen_incidentTo, &incidentTo,
			                    NULL);

			Raul::Atom tail_atom;
			Raul::Atom head_atom;
			Raul::Atom incidentTo_atom;
			get_atom(tail, tail_atom);
			get_atom(head, head_atom);
			get_atom(incidentTo, incidentTo_atom);
			if (tail_atom.is_valid() && head_atom.is_valid()) {
				_iface.disconnect(Raul::Path(tail_atom.get_uri()),
				                  Raul::Path(head_atom.get_uri()));
			} else if (incidentTo_atom.is_valid()) {
				_iface.disconnect_all(subject_uri,
				                      Raul::Path(incidentTo_atom.get_uri()));
			} else {
				Raul::warn << "Delete of unknown object." << std::endl;
				return false;
			}
		}
	} else if (obj->body.otype == _uris.patch_Put) {
		const LV2_Atom_Object* body = NULL;
		lv2_atom_object_get(obj, (LV2_URID)_uris.patch_body, &body, 0);
		if (!body) {
			Raul::warn << "Put message has no body" << std::endl;
			return false;
		} else if (!subject_uri) {
			Raul::warn << "Put message has no subject" << std::endl;
			return false;
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
				return false;
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
			return false;
		} else if (!subject_uri) {
			Raul::warn << "Set message has no subject" << std::endl;
			return false;
		}

		LV2_ATOM_OBJECT_FOREACH(body, p) {
			Raul::Atom val;
			get_atom(&p->value, val);
			_iface.set_property(subject_uri, _map.unmap_uri(p->key), val);
		}
	} else if (obj->body.otype == _uris.patch_Patch) {
		if (!subject_uri) {
			Raul::warn << "Put message has no subject" << std::endl;
			return false;
		}

		const LV2_Atom_Object* remove = NULL;
		lv2_atom_object_get(obj, (LV2_URID)_uris.patch_remove, &remove, 0);
		if (!remove) {
			Raul::warn << "Patch message has no remove" << std::endl;
			return false;
		}

		const LV2_Atom_Object* add = NULL;
		lv2_atom_object_get(obj, (LV2_URID)_uris.patch_add, &add, 0);
		if (!add) {
			Raul::warn << "Patch message has no add" << std::endl;
			return false;
		}

		Ingen::Resource::Properties add_props;
		get_props(remove, add_props);

		Ingen::Resource::Properties remove_props;
		get_props(remove, remove_props);

		_iface.delta(subject_uri, remove_props, add_props);
	} else if (obj->body.otype == _uris.patch_Move) {
		if (!subject_uri) {
			Raul::warn << "Move message has no subject" << std::endl;
			return false;
		}

		const LV2_Atom* dest = NULL;
		lv2_atom_object_get(obj, (LV2_URID)_uris.patch_destination, &dest, 0);
		if (!dest) {
			Raul::warn << "Move message has no destination" << std::endl;
			return false;
		}

		const char* dest_uri = atom_to_uri(dest);
		if (!dest_uri) {
			Raul::warn << "Move message destination is not a URI" << std::endl;
			return false;
		}

		_iface.move(subject_uri, dest_uri);
	} else if (obj->body.otype == _uris.patch_Response) {
		const LV2_Atom* request = NULL;
		const LV2_Atom* body    = NULL;
		lv2_atom_object_get(obj,
		                    (LV2_URID)_uris.patch_request, &request,
		                    (LV2_URID)_uris.patch_body, &body,
		                    0);
		if (!request || request->type != _uris.atom_Int) {
			Raul::warn << "Response message has no request" << std::endl;
			return false;
		} else if (!body || body->type != _uris.atom_Int) {
			Raul::warn << "Response message body is not integer" << std::endl;
			return false;
		}
		_iface.response(((LV2_Atom_Int*)request)->body,
		                (Ingen::Status)((LV2_Atom_Int*)body)->body,
		                subject_uri);
	} else {
		Raul::warn << "Unknown object type <"
		           << _map.unmap_uri(obj->body.otype)
		           << ">" << std::endl;
	}

	return true;
}

} // namespace Shared
} // namespace Ingen
