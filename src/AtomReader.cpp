/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

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

#include "ingen/AtomReader.hpp"
#include "ingen/Log.hpp"
#include "ingen/Node.hpp"
#include "ingen/URIMap.hpp"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"
#include "raul/Path.hpp"

namespace Ingen {

AtomReader::AtomReader(URIMap&    map,
                       URIs&      uris,
                       Log&       log,
                       Forge&     forge,
                       Interface& iface)
	: _map(map)
	, _uris(uris)
	, _log(log)
	, _forge(forge)
	, _iface(iface)
{
}

void
AtomReader::get_atom(const LV2_Atom* in, Atom& out)
{
	if (in) {
		if (in->type == _uris.atom_URID) {
			const LV2_Atom_URID* urid = (const LV2_Atom_URID*)in;
			const char* uri           = _map.unmap_uri(urid->body);
			if (uri) {
				out = _forge.alloc_uri(_map.unmap_uri(urid->body));
			} else {
				_log.error(fmt("Unable to unmap URID %1%\n") % urid->body);
			}
		} else {
			out = _forge.alloc(in->size, in->type, LV2_ATOM_BODY_CONST(in));
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
		Atom val;
		get_atom(&p->value, val);
		props.insert(std::make_pair(Raul::URI(_map.unmap_uri(p->key)), val));
	}
}

boost::optional<Raul::URI>
AtomReader::atom_to_uri(const LV2_Atom* atom)
{
	if (!atom) {
		return boost::optional<Raul::URI>();
	} else if (atom->type == _uris.atom_URI) {
		const char* str = (const char*)LV2_ATOM_BODY_CONST(atom);
		if (Raul::URI::is_valid(str)) {
			return Raul::URI(str);
		} else {
			_log.warn(fmt("Invalid URI <%1%>\n") % str);
		}
	} else if (atom->type == _uris.atom_Path) {
		const char* str = (const char*)LV2_ATOM_BODY_CONST(atom);
		if (!strncmp(str, "file://", 5)) {
			return Raul::URI(str);
		} else {
			return Raul::URI(std::string("file://") + str);
		}
	} else if (atom->type == _uris.atom_URID) {
		const char* str = _map.unmap_uri(((const LV2_Atom_URID*)atom)->body);
		if (str) {
			return Raul::URI(str);
		} else {
			_log.warn(fmt("Unknown URID %1%\n") % str);
		}
	}
	return boost::optional<Raul::URI>();
}

boost::optional<Raul::Path>
AtomReader::atom_to_path(const LV2_Atom* atom)
{
	boost::optional<Raul::URI> uri = atom_to_uri(atom);
	if (uri && Node::uri_is_path(*uri)) {
		return Node::uri_to_path(*uri);
	}
	return boost::optional<Raul::Path>();
}

bool
AtomReader::is_message(URIs& uris, const LV2_Atom* msg)
{
	if (msg->type != uris.atom_Object) {
		return false;
	}

	const LV2_Atom_Object* obj = (const LV2_Atom_Object*)msg;
	return (obj->body.otype == uris.patch_Get ||
	        obj->body.otype == uris.patch_Delete ||
	        obj->body.otype == uris.patch_Put ||
	        obj->body.otype == uris.patch_Set ||
	        obj->body.otype == uris.patch_Patch ||
	        obj->body.otype == uris.patch_Move ||
	        obj->body.otype == uris.patch_Response);
}

bool
AtomReader::write(const LV2_Atom* msg)
{
	if (msg->type != _uris.atom_Object) {
		_log.warn(fmt("Unknown message type <%1%>\n")
		          % _map.unmap_uri(msg->type));
		return false;
	}

	const LV2_Atom_Object* obj     = (const LV2_Atom_Object*)msg;
	const LV2_Atom*        subject = NULL;
	const LV2_Atom*        number  = NULL;

	lv2_atom_object_get(obj,
	                    (LV2_URID)_uris.patch_subject,        &subject,
	                    (LV2_URID)_uris.patch_sequenceNumber, &number,
	                    NULL);

	const boost::optional<Raul::URI> subject_uri = atom_to_uri(subject);

	const int32_t seq_id = ((number && number->type == _uris.atom_Int)
	                        ? ((const LV2_Atom_Int*)number)->body
	                        : 0);
	_iface.set_response_id(seq_id);

	if (obj->body.otype == _uris.patch_Get) {
		if (subject_uri) {
			_iface.get(*subject_uri);
		}
	} else if (obj->body.otype == _uris.patch_Delete) {
		const LV2_Atom_Object* body = NULL;
		lv2_atom_object_get(obj, (LV2_URID)_uris.patch_body, &body, 0);

		if (subject_uri && !body) {
			_iface.del(*subject_uri);
			return true;
		} else if (body && body->body.otype == _uris.ingen_Arc) {
			const LV2_Atom* tail       = NULL;
			const LV2_Atom* head       = NULL;
			const LV2_Atom* incidentTo = NULL;
			lv2_atom_object_get(body,
			                    (LV2_URID)_uris.ingen_tail, &tail,
			                    (LV2_URID)_uris.ingen_head, &head,
			                    (LV2_URID)_uris.ingen_incidentTo, &incidentTo,
			                    NULL);

			boost::optional<Raul::Path> subject_path(atom_to_path(subject));
			boost::optional<Raul::Path> tail_path(atom_to_path(tail));
			boost::optional<Raul::Path> head_path(atom_to_path(head));
			boost::optional<Raul::Path> other_path(atom_to_path(incidentTo));
			if (tail_path && head_path) {
				_iface.disconnect(*tail_path, *head_path);
			} else if (subject_path && other_path) {
				_iface.disconnect_all(*subject_path, *other_path);
			} else {
				_log.warn("Delete of unknown object\n");
				return false;
			}
		}
	} else if (obj->body.otype == _uris.patch_Put) {
		const LV2_Atom_Object* body = NULL;
		lv2_atom_object_get(obj, (LV2_URID)_uris.patch_body, &body, 0);
		if (!body) {
			_log.warn("Put message has no body\n");
			return false;
		} else if (!subject_uri) {
			_log.warn("Put message has no subject\n");
			return false;
		}

		if (body->body.otype == _uris.ingen_Arc) {
			LV2_Atom* tail = NULL;
			LV2_Atom* head = NULL;
			lv2_atom_object_get(body,
			                    (LV2_URID)_uris.ingen_tail, &tail,
			                    (LV2_URID)_uris.ingen_head, &head,
			                    NULL);
			if (!tail || !head) {
				_log.warn("Arc has no tail or head\n");
				return false;
			}

			boost::optional<Raul::Path> tail_path(atom_to_path(tail));
			boost::optional<Raul::Path> head_path(atom_to_path(head));
			if (tail_path && head_path) {
				_iface.connect(*tail_path, *head_path);
			} else {
				_log.warn("Arc has non-path tail or head\n");
			}
		} else {
			Ingen::Resource::Properties props;
			get_props(body, props);
			_iface.put(*subject_uri, props);
		}
	} else if (obj->body.otype == _uris.patch_Set) {
		if (!subject_uri) {
			_log.warn("Set message has no subject\n");
			return false;
		}

		const LV2_Atom_URID* prop = NULL;
		lv2_atom_object_get(obj, (LV2_URID)_uris.patch_property, &prop, 0);
		if (!prop || ((const LV2_Atom*)prop)->type != _uris.atom_URID) {
			_log.warn("Set message missing property\n");
			return false;
		}

		const LV2_Atom* value = NULL;
		lv2_atom_object_get(obj, (LV2_URID)_uris.patch_value, &value, 0);
		if (!value) {
			_log.warn("Set message missing value\n");
			return false;
		}

		Atom atom;
		get_atom(value, atom);
		_iface.set_property(*subject_uri,
		                    Raul::URI(_map.unmap_uri(prop->body)),
		                    atom);
	} else if (obj->body.otype == _uris.patch_Patch) {
		if (!subject_uri) {
			_log.warn("Patch message has no subject\n");
			return false;
		}

		const LV2_Atom_Object* remove = NULL;
		lv2_atom_object_get(obj, (LV2_URID)_uris.patch_remove, &remove, 0);
		if (!remove) {
			_log.warn("Patch message has no remove\n");
			return false;
		}

		const LV2_Atom_Object* add = NULL;
		lv2_atom_object_get(obj, (LV2_URID)_uris.patch_add, &add, 0);
		if (!add) {
			_log.warn("Patch message has no add\n");
			return false;
		}

		Ingen::Resource::Properties add_props;
		get_props(add, add_props);

		Ingen::Resource::Properties remove_props;
		get_props(remove, remove_props);

		_iface.delta(*subject_uri, remove_props, add_props);
	} else if (obj->body.otype == _uris.patch_Copy) {
		if (!subject) {
			_log.warn("Copy message has no subject\n");
			return false;
		}

		const LV2_Atom* dest = NULL;
		lv2_atom_object_get(obj, (LV2_URID)_uris.patch_destination, &dest, 0);
		if (!dest) {
			_log.warn("Copy message has no destination\n");
			return false;
		}

		boost::optional<Raul::URI> subject_uri(atom_to_uri(subject));
		if (!subject_uri) {
			_log.warn("Copy message has non-path subject\n");
			return false;
		}

		boost::optional<Raul::URI> dest_uri(atom_to_uri(dest));
		if (!dest_uri) {
			_log.warn("Copy message has non-URI destination\n");
			return false;
		}

		_iface.copy(*subject_uri, *dest_uri);
	} else if (obj->body.otype == _uris.patch_Move) {
		if (!subject) {
			_log.warn("Move message has no subject\n");
			return false;
		}

		const LV2_Atom* dest = NULL;
		lv2_atom_object_get(obj, (LV2_URID)_uris.patch_destination, &dest, 0);
		if (!dest) {
			_log.warn("Move message has no destination\n");
			return false;
		}

		boost::optional<Raul::Path> subject_path(atom_to_path(subject));
		if (!subject_path) {
			_log.warn("Move message has non-path subject\n");
			return false;
		}

		boost::optional<Raul::Path> dest_path(atom_to_path(dest));
		if (!dest_path) {
			_log.warn("Move message has non-path destination\n");
			return false;
		}

		_iface.move(*subject_path, *dest_path);
	} else if (obj->body.otype == _uris.patch_Response) {
		const LV2_Atom* request = NULL;
		const LV2_Atom* body    = NULL;
		lv2_atom_object_get(obj,
		                    (LV2_URID)_uris.patch_request, &request,
		                    (LV2_URID)_uris.patch_body, &body,
		                    0);
		if (!request || request->type != _uris.atom_Int) {
			_log.warn("Response message has no request\n");
			return false;
		} else if (!body || body->type != _uris.atom_Int) {
			_log.warn("Response message body is not integer\n");
			return false;
		}
		_iface.response(((const LV2_Atom_Int*)request)->body,
		                (Ingen::Status)((const LV2_Atom_Int*)body)->body,
		                subject_uri ? subject_uri->c_str() : "");
	} else {
		_log.warn(fmt("Unknown object type <%1%>\n")
		          % _map.unmap_uri(obj->body.otype));
	}

	return true;
}

} // namespace Ingen
