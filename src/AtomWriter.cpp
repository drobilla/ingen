/*
  This file is part of Ingen.
  Copyright 2007-2016 David Robillard <http://drobilla.net/>

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
#include <cstdlib>
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

AtomWriter::~AtomWriter()
{
	free((void*)_out.buf);
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
	LV2_Atom_Forge_Frame msg;
	forge_request(&msg, _uris.ingen_BundleStart);
	finish_msg();
}

void
AtomWriter::bundle_end()
{
	LV2_Atom_Forge_Frame msg;
	forge_request(&msg, _uris.ingen_BundleEnd);
	finish_msg();
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

/** @page protocol Ingen Protocol
 * @tableofcontents
 *
 * @section methods Methods
 */

/** @page protocol
 * @subsection Put
 *
 * Use [patch:Put](http://lv2plug.in/ns/ext/patch#Put) to set properties on an
 * object, creating it if necessary.
 *
 * If the object already exists, all existing object properties with keys that
 * match any property in the message are first removed.  Other properties are
 * left unchanged.
 *
 * If the object does not yet exist, the message must contain sufficient
 * information to create it (e.g. supported rdf:type properties).
 *
 * @code{.ttl}
 * []
 *     a patch:Put ;
 *     patch:subject </graph/osc> ;
 *     patch:body [
 *         a ingen:Block ;
 *         lv2:prototype <http://drobilla.net/plugins/mda/Shepard>
 *     ] .
 * @endcode
 */
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

/** @page protocol
 * @subsection Patch
 *
 * Use [patch:Patch](http://lv2plug.in/ns/ext/patch#Patch) to manipulate the
 * properties of an object.  A set of properties are first removed, then
 * another is added.  Analogous to WebDAV PROPPATCH.
 *
 * The special value [patch:wildcard](http://lv2plug.in/ns/ext/patch#wildcard)
 * may be used to specify that any value with the given key should be removed.
 *
 * @code{.ttl}
 * []
 *     a patch:Patch ;
 *     patch:subject </graph/osc> ;
 *     patch:add [
 *         lv2:name "Osckillator" ;
 *         ingen:canvasX 32.0 ;
 *         ingen:canvasY 32.0 ;
 *     ] ;
 *     patch:remove [
 *         eg:name "Old name" ;            # Remove specific value
 *         ingen:canvasX patch:wildcard ;  # Remove all
 *         ingen:canvasY patch:wildcard ;  # Remove all
 *     ] .
 * @endcode
 */
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

/** @page protocol
 * @subsection Copy
 *
 * Use [patch:Copy](http://lv2plug.in/ns/ext/copy#Copy) to copy an object from
 * its current location (subject) to another (destination).
 *
 * If both the subject and destination are inside Ingen, like block paths, then the old object
 * is copied by, for example, creating a new plugin instance.
 *
 * If the subject is a path and the destination is inside Ingen, then the
 * subject must be an Ingen graph file or bundle, which is loaded to the
 * specified destination path.
 *
 * If the subject is inside Ingen and the destination is a path, then the
 * subject is saved to an Ingen bundle at the given destination.
 *
 * @code{.ttl}
 * []
 *     a patch:Copy ;
 *     patch:subject </graph/osc> ;
 *     patch:destination </graph/osc2> .
 * @endcode
 */
void
AtomWriter::copy(const Raul::URI& old_uri,
                 const Raul::URI& new_uri)
{
	LV2_Atom_Forge_Frame msg;
	forge_request(&msg, _uris.patch_Copy);
	lv2_atom_forge_key(&_forge, _uris.patch_subject);
	forge_uri(old_uri);
	lv2_atom_forge_key(&_forge, _uris.patch_destination);
	forge_uri(new_uri);
	lv2_atom_forge_pop(&_forge, &msg);
	finish_msg();
}

/** @page protocol
 * @subsection Move
 *
 * Use [patch:Move](http://lv2plug.in/ns/ext/move#Move) to move an object from
 * its current location (subject) to another (destination).
 *
 * Both subject and destination must be paths in Ingen with the same parent,
 * moving between graphs is currently not supported.
 *
 * @code{.ttl}
 * []
 *     a patch:Move ;
 *     patch:subject </graph/osc> ;
 *     patch:destination </graph/osc2> .
 * @endcode
 */
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

/** @page protocol
 * @subsection Delete
 *
 * Use [patch:Delete](http://lv2plug.in/ns/ext/delete#Delete) to remove an
 * object from the engine and destroy it.
 *
 * All properties of the object are lost, as are all references to the object
 * (e.g. any connections to it).
 *
 * @code{.ttl}
 * []
 *     a patch:Delete ;
 *     patch:subject </graph/osc> .
 * @endcode
 */
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

/** @page protocol
 * @subsection Set
 *
 * Use [patch:Set](http://lv2plug.in/ns/ext/patch#Set) to set a property on an
 * object.  Any existing value for that property is removed.
 *
 * @code{.ttl}
 * []
 *     a patch:Set ;
 *     patch:subject </graph/osc> ;
 *     patch:property lv2:name ;
 *     patch:value "Oscwellator" .
 * @endcode
 */
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

/** @page protocol
 * @subsection Undo
 *
 * Use [ingen:Undo](http://drobilla.net/ns/ingen#Undo) to undo the last change
 * to the engine.
 *
 * @code{.ttl}
 * [] a ingen:Undo .
 * @endcode
 */
void
AtomWriter::undo()
{
	LV2_Atom_Forge_Frame msg;
	forge_request(&msg, _uris.ingen_Undo);
	lv2_atom_forge_pop(&_forge, &msg);
	finish_msg();
}

/** @page protocol
 * @subsection Undo
 *
 * Use [ingen:Redo](http://drobilla.net/ns/ingen#Redo) to undo the last change
 * to the engine.
 *
 * @code{.ttl}
 * [] a ingen:Redo .
 * @endcode
 */
void
AtomWriter::redo()
{
	LV2_Atom_Forge_Frame msg;
	forge_request(&msg, _uris.ingen_Redo);
	lv2_atom_forge_pop(&_forge, &msg);
	finish_msg();
}

/** @page protocol
 * @subsection Get
 *
 * Use [patch:Get](http://lv2plug.in/ns/ext/patch#Get) to get the description
 * of the subject.
 *
 * @code{.ttl}
 * []
 *     a patch:Get ;
 *     patch:subject </graph/osc> .
 * @endcode
 */
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

/** @page protocol
 *
 * @section arcs Arc Manipulation
 */

/** @page protocol
 * @subsection Connect Connecting Two Ports
 *
 * Ports are connected by putting an arc with the desired tail (an output port)
 * and head (an input port).  The tail and head must both be within the
 * subject, which must be a graph.
 *
 * @code{.ttl}
 * []
 *     a patch:Put ;
 *     patch:subject </graph/> ;
 *     patch:body [
 *         a ingen:Arc ;
 *         ingen:tail </graph/osc/out> ;
 *         ingen:head </graph/filt/in> ;
 *     ] .
 * @endcode
 */
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

/** @page protocol
 * @subsection Disconnect Disconnecting Two Ports
 *
 * Ports are disconnected by deleting the arc between them.  The description of
 * the arc is the same as in the put command used to create the connection.
 *
 * @code{.ttl}
 * []
 *     a patch:Delete ;
 *     patch:body [
 *         a ingen:Arc ;
 *         ingen:tail </graph/osc/out> ;
 *         ingen:head </graph/filt/in> ;
 *     ] .
 * @endcode
 */
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

/** @page protocol
 * @subsection DisconnectAll Fully Disconnecting an Object
 *
 * Disconnect a port completely is similar to disconnecting a specific port,
 * but rather than specifying a specific tail and head, the special property
 * ingen:incidentTo is used to specify any arc that is connected to a port or
 * block in either direction.  This works with ports and blocks (including
 * graphs, which act as blocks for the purpose of this operation and are not
 * modified internally).
 *
 * @code{.ttl}
 * []
 *     a patch:Delete ;
 *     patch:subject </graph> ;
 *     patch:body [
 *         a ingen:Arc ;
 *         ingen:incidentTo </graph/osc/out>
 *     ] .
 * @endcode
 */
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
AtomWriter::set_response_id(int32_t id)
{
	_id = id;
}


/** @page protocol
 * @section Responses
 *
 * Ingen responds to requests if the patch:sequenceNumber property is set.  For
 * example:
 * @code{.ttl}
 * []
 *     a patch:Get ;
 *     patch:sequenceNumber 42 ;
 *     patch:subject </graph/osc> .
 * @endcode
 *
 * Might receive a response like:
 * @code{.ttl}
 * []
 *     a patch:Response ;
 *     patch:sequenceNumber 42 ;
 *     patch:subject </graph/osc> ;
 *     patch:body 0 .
 * @endcode
 *
 * Where 0 is a status code, 0 meaning success and any other value being an
 * error.  Information about status codes, including error message strings,
 * are defined in ingen.lv2/errors.ttl.
 *
 * Note that a response is only a status response, operations that manipulate
 * the graph may generate new data on the stream, e.g. the above get request
 * would also receive a put that describes /graph/osc in the stream immediately
 * following the response.
 */
void
AtomWriter::response(int32_t id, Status status, const std::string& subject)
{
	if (!id) {
		return;
	}

	LV2_Atom_Forge_Frame msg;
	forge_request(&msg, _uris.patch_Response);
	lv2_atom_forge_key(&_forge, _uris.patch_sequenceNumber);
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

/** @page protocol
 * @tableofcontents
 */

} // namespace Ingen
