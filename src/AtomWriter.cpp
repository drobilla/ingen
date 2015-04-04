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

#include <cassert>
#include <string>

#include "ingen/AtomSink.hpp"
#include "ingen/AtomWriter.hpp"
#include "ingen/Node.hpp"
#include "ingen/URIMap.hpp"
#include "raul/Path.hpp"
#include "serd/serd.h"

namespace Ingen {

static LV2_Atom_Forge_Ref
forge_sink(LV2_Atom_Forge_Sink_Handle handle,
           const void*                buf,
           uint32_t                   size)
{
	SerdChunk*               chunk = (SerdChunk*)handle;
	const LV2_Atom_Forge_Ref ref   = chunk->len + 1;
	serd_chunk_sink(buf, size, chunk);
	return ref;
}

static LV2_Atom*
forge_deref(LV2_Atom_Forge_Sink_Handle handle, LV2_Atom_Forge_Ref ref)
{
	SerdChunk* chunk = (SerdChunk*)handle;
	return (LV2_Atom*)(chunk->buf + ref - 1);
}

AtomWriter::AtomWriter(URIMap& map, URIs& uris, AtomSink& sink)
	: _map(map)
	, _uris(uris)
	, _sink(sink)
	, _id(0)
{
	_out.buf = NULL;
	_out.len = 0;
	lv2_atom_forge_init(&_forge, &map.urid_map_feature()->urid_map);
	lv2_atom_forge_set_sink(&_forge, forge_sink, forge_deref, &_out);
}

void
AtomWriter::finish_msg()
{
	assert(!_forge.stack);
	_sink.write((const LV2_Atom*)_out.buf);
	_out.len = 0;
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
AtomWriter::forge_uri(const Raul::URI& uri)
{
	if (serd_uri_string_has_scheme((const uint8_t*)uri.c_str())) {
		lv2_atom_forge_urid(&_forge, _map.map_uri(uri.c_str()));
	} else {
		lv2_atom_forge_uri(&_forge, uri.c_str(), uri.length());
	}
}

void
AtomWriter::forge_properties(const Resource::Properties& properties)
{
	for (auto p : properties) {
		lv2_atom_forge_key(&_forge, _map.map_uri(p.first.c_str()));
		if (p.second.type() == _forge.URI) {
			forge_uri(Raul::URI(p.second.ptr<char>()));
		} else {
			lv2_atom_forge_atom(&_forge, p.second.size(), p.second.type());
			lv2_atom_forge_write(&_forge, p.second.get_body(), p.second.size());
		}
	}
}

void
AtomWriter::forge_arc(const Raul::Path& tail, const Raul::Path& head)
{
	LV2_Atom_Forge_Frame arc;
	lv2_atom_forge_object(&_forge, &arc, 0, _uris.ingen_Arc);
	lv2_atom_forge_key(&_forge, _uris.ingen_tail);
	forge_uri(Node::path_to_uri(tail));
	lv2_atom_forge_key(&_forge, _uris.ingen_head);
	forge_uri(Node::path_to_uri(head));
	lv2_atom_forge_pop(&_forge, &arc);
}

void
AtomWriter::forge_request(LV2_Atom_Forge_Frame* frame, LV2_URID type)
{
	lv2_atom_forge_object(&_forge, frame, 0, type);

	if (_id) {
		lv2_atom_forge_key(&_forge, _uris.patch_sequenceNumber);
		lv2_atom_forge_int(&_forge, _id);
	}
}

void
AtomWriter::put(const Raul::URI&            uri,
                const Resource::Properties& properties,
                Resource::Graph             ctx)
{
	LV2_Atom_Forge_Frame msg;
	forge_request(&msg, _uris.patch_Put);
	lv2_atom_forge_key(&_forge, _uris.patch_subject);
	forge_uri(uri);
	lv2_atom_forge_key(&_forge, _uris.patch_body);

	LV2_Atom_Forge_Frame body;
	lv2_atom_forge_object(&_forge, &body, 0, 0);
	forge_properties(properties);
	lv2_atom_forge_pop(&_forge, &body);

	lv2_atom_forge_pop(&_forge, &msg);
	finish_msg();
}

void
AtomWriter::delta(const Raul::URI&            uri,
                  const Resource::Properties& remove,
                  const Resource::Properties& add)
{
	LV2_Atom_Forge_Frame msg;
	forge_request(&msg, _uris.patch_Patch);
	lv2_atom_forge_key(&_forge, _uris.patch_subject);
	forge_uri(uri);

	lv2_atom_forge_key(&_forge, _uris.patch_remove);
	LV2_Atom_Forge_Frame remove_obj;
	lv2_atom_forge_object(&_forge, &remove_obj, 0, 0);
	forge_properties(remove);
	lv2_atom_forge_pop(&_forge, &remove_obj);

	lv2_atom_forge_key(&_forge, _uris.patch_add);
	LV2_Atom_Forge_Frame add_obj;
	lv2_atom_forge_object(&_forge, &add_obj, 0, 0);
	forge_properties(add);
	lv2_atom_forge_pop(&_forge, &add_obj);

	lv2_atom_forge_pop(&_forge, &msg);
	finish_msg();
}

void
AtomWriter::copy(const Raul::Path& old_path,
                 const Raul::URI&  new_uri)
{
	LV2_Atom_Forge_Frame msg;
	forge_request(&msg, _uris.patch_Copy);
	lv2_atom_forge_key(&_forge, _uris.patch_subject);
	forge_uri(Node::path_to_uri(old_path));
	lv2_atom_forge_key(&_forge, _uris.patch_destination);
	forge_uri(new_uri);
	lv2_atom_forge_pop(&_forge, &msg);
	finish_msg();
}

void
AtomWriter::move(const Raul::Path& old_path,
                 const Raul::Path& new_path)
{
	LV2_Atom_Forge_Frame msg;
	forge_request(&msg, _uris.patch_Move);
	lv2_atom_forge_key(&_forge, _uris.patch_subject);
	forge_uri(Node::path_to_uri(old_path));
	lv2_atom_forge_key(&_forge, _uris.patch_destination);
	forge_uri(Node::path_to_uri(new_path));
	lv2_atom_forge_pop(&_forge, &msg);
	finish_msg();
}

void
AtomWriter::del(const Raul::URI& uri)
{
	LV2_Atom_Forge_Frame msg;
	forge_request(&msg, _uris.patch_Delete);
	lv2_atom_forge_key(&_forge, _uris.patch_subject);
	forge_uri(uri);
	lv2_atom_forge_pop(&_forge, &msg);
	finish_msg();
}

void
AtomWriter::connect(const Raul::Path& tail,
                    const Raul::Path& head)
{
	LV2_Atom_Forge_Frame msg;
	forge_request(&msg, _uris.patch_Put);
	lv2_atom_forge_key(&_forge, _uris.patch_subject);
	forge_uri(Node::path_to_uri(Raul::Path::lca(tail, head)));
	lv2_atom_forge_key(&_forge, _uris.patch_body);
	forge_arc(tail, head);
	lv2_atom_forge_pop(&_forge, &msg);
	finish_msg();
}

void
AtomWriter::disconnect(const Raul::Path& tail,
                       const Raul::Path& head)
{
	LV2_Atom_Forge_Frame msg;
	forge_request(&msg, _uris.patch_Delete);
	lv2_atom_forge_key(&_forge, _uris.patch_body);
	forge_arc(tail, head);
	lv2_atom_forge_pop(&_forge, &msg);
	finish_msg();
}

void
AtomWriter::disconnect_all(const Raul::Path& graph,
                           const Raul::Path& path)
{
	LV2_Atom_Forge_Frame msg;
	forge_request(&msg, _uris.patch_Delete);

	lv2_atom_forge_key(&_forge, _uris.patch_subject);
	forge_uri(Node::path_to_uri(graph));

	lv2_atom_forge_key(&_forge, _uris.patch_body);
	LV2_Atom_Forge_Frame arc;
	lv2_atom_forge_object(&_forge, &arc, 0, _uris.ingen_Arc);
	lv2_atom_forge_key(&_forge, _uris.ingen_incidentTo);
	forge_uri(Node::path_to_uri(path));
	lv2_atom_forge_pop(&_forge, &arc);

	lv2_atom_forge_pop(&_forge, &msg);
	finish_msg();
}

void
AtomWriter::set_property(const Raul::URI& subject,
                         const Raul::URI& predicate,
                         const Atom&      value)
{
	LV2_Atom_Forge_Frame msg;
	forge_request(&msg, _uris.patch_Set);
	lv2_atom_forge_key(&_forge, _uris.patch_subject);
	forge_uri(subject);
	lv2_atom_forge_key(&_forge, _uris.patch_property);
	lv2_atom_forge_urid(&_forge, _map.map_uri(predicate.c_str()));
	lv2_atom_forge_key(&_forge, _uris.patch_value);
	lv2_atom_forge_atom(&_forge, value.size(), value.type());
	lv2_atom_forge_write(&_forge, value.get_body(), value.size());

	lv2_atom_forge_pop(&_forge, &msg);
	finish_msg();
}

void
AtomWriter::set_response_id(int32_t id)
{
	_id = id;
}

void
AtomWriter::get(const Raul::URI& uri)
{
	LV2_Atom_Forge_Frame msg;
	forge_request(&msg, _uris.patch_Get);
	lv2_atom_forge_key(&_forge, _uris.patch_subject);
	forge_uri(uri);
	lv2_atom_forge_pop(&_forge, &msg);
	finish_msg();
}

void
AtomWriter::response(int32_t id, Status status, const std::string& subject)
{
	if (!id) {
		return;
	}

	LV2_Atom_Forge_Frame msg;
	forge_request(&msg, _uris.patch_Response);
	lv2_atom_forge_key(&_forge, _uris.patch_request);
	lv2_atom_forge_int(&_forge, id);
	if (!subject.empty() && Raul::URI::is_valid(subject)) {
		lv2_atom_forge_key(&_forge, _uris.patch_subject);
		lv2_atom_forge_uri(&_forge, subject.c_str(), subject.length());
	}
	lv2_atom_forge_key(&_forge, _uris.patch_body);
	lv2_atom_forge_int(&_forge, static_cast<int>(status));
	lv2_atom_forge_pop(&_forge, &msg);
	finish_msg();
}

void
AtomWriter::error(const std::string& msg)
{
}

} // namespace Ingen
