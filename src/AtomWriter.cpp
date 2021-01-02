/*
  This file is part of Ingen.
  Copyright 2007-2017 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

/** @page protocol Ingen Protocol
 * @tableofcontents
 *
 * @section introduction Controlling Ingen
 *
 * Ingen is controlled via a simple RDF-based protocol.  This conceptual
 * protocol can be used in two concrete ways:
 *
 * 1. When Ingen is running as a process, a socket accepts messages in
 * (textual) Turtle syntax, and responds in the same syntax.  Transfers are
 * delimited by null characters in the stream, so the client knows when to
 * finish parsing to interpret responses.  By default, Ingen listens on
 * unix:///tmp/ingen.sock and tcp://localhost:16180
 *
 * 2. When Ingen is running as an LV2 plugin, the control and notify ports
 * accept and respond using (binary) LV2 atoms.  Messages are read and written
 * as events with atom:Object bodies.  The standard rules for LV2 event
 * transmission apply, but note that the graph manipulations described here are
 * executed asynchronously and not with sample accuracy, so the response may
 * come at a later frame or cycle.
 *
 * For documentation purposes, this page discusses messages in Turtle syntax,
 * but the same protocol is used in the LV2 case, just in a more compact binary
 * encoding.
 *
 * Conceptually, Ingen represents a tree of objects, each of which has a path
 * (like /main/in or /main/osc/out) and a set of properties.  A property is a
 * URI key and some value.  All changes to Ingen are represented as
 * manipulations of this tree of dictionaries.  The core of the protocol is
 * based on the LV2 patch extension, which defines several messages for
 * manipulating data in this model which resemble HTTP methods.
 */

#include "ingen/AtomWriter.hpp"

#include "ingen/Atom.hpp"
#include "ingen/AtomForge.hpp"
#include "ingen/AtomSink.hpp"
#include "ingen/Message.hpp"
#include "ingen/Properties.hpp"
#include "ingen/Resource.hpp"
#include "ingen/URI.hpp"
#include "ingen/URIMap.hpp"
#include "ingen/URIs.hpp"
#include "ingen/paths.hpp"
#include "lv2/atom/forge.h"
#include "lv2/urid/urid.h"
#include "raul/Path.hpp"
#include "serd/serd.h"

#include <boost/variant/apply_visitor.hpp>

#include <cassert>
#include <cstdint>
#include <map>
#include <string>
#include <utility>

namespace ingen {

AtomWriter::AtomWriter(URIMap& map, URIs& uris, AtomSink& sink)
	: _map(map)
	, _uris(uris)
	, _sink(sink)
	, _forge(map.urid_map())
{
}

void
AtomWriter::finish_msg()
{
	assert(!_forge.stack);
	_sink.write(_forge.atom());
	_forge.clear();
}

void
AtomWriter::message(const Message& message)
{
	boost::apply_visitor(*this, message);
}

/** @page protocol
 * @subsection Bundles
 *
 * An [ingen:BundleStart](http://drobilla.net/ns/ingen#BundleStart) marks the
 * start of a bundle in the operation stream.  A bundle groups a series of
 * messages for coarse-grained undo or atomic execution.
 *
 * @code{.ttl}
 * [] a ingen:BundleStart .
 * @endcode

 * An [ingen:BundleEnd](http://drobilla.net/ns/ingen#BundleEnd) marks the end
 * of a bundle in the operation stream.
 *
 * @code{.ttl}
 * [] a ingen:BundleEnd .
 * @endcode
 */
void
AtomWriter::operator()(const BundleBegin& message)
{
	LV2_Atom_Forge_Frame msg;
	forge_request(&msg, _uris.ingen_BundleStart, message.seq);
	lv2_atom_forge_pop(&_forge, &msg);
	finish_msg();
}

void
AtomWriter::operator()(const BundleEnd& message)
{
	LV2_Atom_Forge_Frame msg;
	forge_request(&msg, _uris.ingen_BundleEnd, message.seq);
	lv2_atom_forge_pop(&_forge, &msg);
	finish_msg();
}

void
AtomWriter::forge_uri(const URI& uri)
{
	if (serd_uri_string_has_scheme(
	        reinterpret_cast<const uint8_t*>(uri.c_str()))) {
		lv2_atom_forge_urid(&_forge, _map.map_uri(uri.c_str()));
	} else {
		lv2_atom_forge_uri(&_forge, uri.c_str(), uri.length());
	}
}

void
AtomWriter::forge_properties(const Properties& properties)
{
	for (auto p : properties) {
		lv2_atom_forge_key(&_forge, _map.map_uri(p.first.c_str()));
		if (p.second.type() == _forge.URI) {
			forge_uri(URI(p.second.ptr<char>()));
		} else {
			lv2_atom_forge_atom(&_forge, p.second.size(), p.second.type());
			lv2_atom_forge_write(&_forge, p.second.get_body(), p.second.size());
		}
	}
}

void
AtomWriter::forge_arc(const raul::Path& tail, const raul::Path& head)
{
	LV2_Atom_Forge_Frame arc;
	lv2_atom_forge_object(&_forge, &arc, 0, _uris.ingen_Arc);
	lv2_atom_forge_key(&_forge, _uris.ingen_tail);
	forge_uri(path_to_uri(tail));
	lv2_atom_forge_key(&_forge, _uris.ingen_head);
	forge_uri(path_to_uri(head));
	lv2_atom_forge_pop(&_forge, &arc);
}

void
AtomWriter::forge_request(LV2_Atom_Forge_Frame* frame, LV2_URID type, int32_t id)
{
	lv2_atom_forge_object(&_forge, frame, 0, type);

	if (id) {
		lv2_atom_forge_key(&_forge, _uris.patch_sequenceNumber);
		lv2_atom_forge_int(&_forge, id);
	}
}

void
AtomWriter::forge_context(Resource::Graph ctx)
{
	switch (ctx) {
	case Resource::Graph::EXTERNAL:
		lv2_atom_forge_key(&_forge, _uris.patch_context);
		forge_uri(_uris.ingen_externalContext);
		break;
	case Resource::Graph::INTERNAL:
		lv2_atom_forge_key(&_forge, _uris.patch_context);
		forge_uri(_uris.ingen_internalContext);
		break;
	default: break;
	}
}

/** @page protocol
 * @section methods Methods
 * @subsection Put
 *
 * Send a [Put](http://lv2plug.in/ns/ext/patch#Put) to set properties on an
 * object, creating it if necessary.
 *
 * If the object already exists, all existing object properties with keys that
 * match any property in the message are first removed.  Other properties are
 * left unchanged.
 *
 * If the object does not yet exist, the message must contain sufficient
 * information to create it, including at least one rdf:type property.
 *
 * @code{.ttl}
 * []
 *     a patch:Put ;
 *     patch:subject </main/osc> ;
 *     patch:body [
 *         a ingen:Block ;
 *         lv2:prototype <http: //drobilla.net/plugins/mda/Shepard>
 *     ] .
 * @endcode
 */
void
AtomWriter::operator()(const Put& message)
{
	LV2_Atom_Forge_Frame msg;
	forge_request(&msg, _uris.patch_Put, message.seq);
	forge_context(message.ctx);
	lv2_atom_forge_key(&_forge, _uris.patch_subject);
	forge_uri(message.uri);
	lv2_atom_forge_key(&_forge, _uris.patch_body);

	LV2_Atom_Forge_Frame body;
	lv2_atom_forge_object(&_forge, &body, 0, 0);
	forge_properties(message.properties);
	lv2_atom_forge_pop(&_forge, &body);

	lv2_atom_forge_pop(&_forge, &msg);
	finish_msg();
}

/** @page protocol
 * @subsection Patch
 *
 * Send a [Patch](http://lv2plug.in/ns/ext/patch#Patch) to manipulate the
 * properties of an object.  A set of properties are first removed, then
 * another is added.  Analogous to WebDAV PROPPATCH.
 *
 * The special value [patch:wildcard](http://lv2plug.in/ns/ext/patch#wildcard)
 * may be used to specify that any value with the given key should be removed.
 *
 * @code{.ttl}
 * []
 *     a patch:Patch ;
 *     patch:subject </main/osc> ;
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
AtomWriter::operator()(const Delta& message)
{
	LV2_Atom_Forge_Frame msg;
	forge_request(&msg, _uris.patch_Patch, message.seq);
	forge_context(message.ctx);
	lv2_atom_forge_key(&_forge, _uris.patch_subject);
	forge_uri(message.uri);

	lv2_atom_forge_key(&_forge, _uris.patch_remove);
	LV2_Atom_Forge_Frame remove_obj;
	lv2_atom_forge_object(&_forge, &remove_obj, 0, 0);
	forge_properties(message.remove);
	lv2_atom_forge_pop(&_forge, &remove_obj);

	lv2_atom_forge_key(&_forge, _uris.patch_add);
	LV2_Atom_Forge_Frame add_obj;
	lv2_atom_forge_object(&_forge, &add_obj, 0, 0);
	forge_properties(message.add);
	lv2_atom_forge_pop(&_forge, &add_obj);

	lv2_atom_forge_pop(&_forge, &msg);
	finish_msg();
}

/** @page protocol
 * @subsection Copy
 *
 * Send a [Copy](http://lv2plug.in/ns/ext/copy#Copy) to copy an object from
 * its current location (subject) to another (destination).
 *
 * If both the subject and destination are inside Ingen, like block paths, then the old object
 * is copied by, for example, creating a new plugin instance.
 *
 * If the subject is a filename (file URI or atom:Path) and the destination is
 * inside Ingen, then the subject must be an Ingen graph file or bundle, which
 * is loaded to the specified destination path.
 *
 * If the subject is inside Ingen and the destination is a filename, then the
 * subject is saved to an Ingen bundle at the given destination.
 *
 * @code{.ttl}
 * []
 *     a patch:Copy ;
 *     patch:subject </main/osc> ;
 *     patch:destination </main/osc2> .
 * @endcode
 */
void
AtomWriter::operator()(const Copy& message)
{
	LV2_Atom_Forge_Frame msg;
	forge_request(&msg, _uris.patch_Copy, message.seq);
	lv2_atom_forge_key(&_forge, _uris.patch_subject);
	forge_uri(message.old_uri);
	lv2_atom_forge_key(&_forge, _uris.patch_destination);
	forge_uri(message.new_uri);
	lv2_atom_forge_pop(&_forge, &msg);
	finish_msg();
}

/** @page protocol
 * @subsection Move
 *
 * Send a [Move](http://lv2plug.in/ns/ext/move#Move) to move an object from its
 * current location (subject) to another (destination).
 *
 * Both subject and destination must be paths in Ingen with the same parent,
 * moving between graphs is currently not supported.
 *
 * @code{.ttl}
 * []
 *     a patch:Move ;
 *     patch:subject </main/osc> ;
 *     patch:destination </main/osc2> .
 * @endcode
 */
void
AtomWriter::operator()(const Move& message)
{
	LV2_Atom_Forge_Frame msg;
	forge_request(&msg, _uris.patch_Move, message.seq);
	lv2_atom_forge_key(&_forge, _uris.patch_subject);
	forge_uri(path_to_uri(message.old_path));
	lv2_atom_forge_key(&_forge, _uris.patch_destination);
	forge_uri(path_to_uri(message.new_path));
	lv2_atom_forge_pop(&_forge, &msg);
	finish_msg();
}

/** @page protocol
 * @subsection Delete
 *
 * Send a [Delete](http://lv2plug.in/ns/ext/delete#Delete) to remove an
 * object from the engine and destroy it.
 *
 * All properties of the object are lost, as are all references to the object
 * (e.g. any connections to it).
 *
 * @code{.ttl}
 * []
 *     a patch:Delete ;
 *     patch:subject </main/osc> .
 * @endcode
 */
void
AtomWriter::operator()(const Del& message)
{
	LV2_Atom_Forge_Frame msg;
	forge_request(&msg, _uris.patch_Delete, message.seq);
	lv2_atom_forge_key(&_forge, _uris.patch_subject);
	forge_uri(message.uri);
	lv2_atom_forge_pop(&_forge, &msg);
	finish_msg();
}

/** @page protocol
 * @subsection Set
 *
 * Send a [Set](http://lv2plug.in/ns/ext/patch#Set) to set a property on an
 * object.  Any existing value for that property is removed.
 *
 * @code{.ttl}
 * []
 *     a patch:Set ;
 *     patch:subject </main/osc> ;
 *     patch:property lv2:name ;
 *     patch:value "Oscwellator" .
 * @endcode
 */
void
AtomWriter::operator()(const SetProperty& message)
{
	LV2_Atom_Forge_Frame msg;
	forge_request(&msg, _uris.patch_Set, message.seq);
	forge_context(message.ctx);
	lv2_atom_forge_key(&_forge, _uris.patch_subject);
	forge_uri(message.subject);
	lv2_atom_forge_key(&_forge, _uris.patch_property);
	lv2_atom_forge_urid(&_forge, _map.map_uri(message.predicate.c_str()));
	lv2_atom_forge_key(&_forge, _uris.patch_value);
	lv2_atom_forge_atom(&_forge, message.value.size(), message.value.type());
	lv2_atom_forge_write(&_forge, message.value.get_body(), message.value.size());

	lv2_atom_forge_pop(&_forge, &msg);
	finish_msg();
}

/** @page protocol
 * @subsection Undo
 *
 * Use [ingen:Undo](http://drobilla.net/ns/ingen#Undo) to undo the last change
 * to the engine.  Undo transactions can be delimited using bundle markers, if
 * the last operations(s) received were in a bundle, then an Undo will undo the
 * effects of that entire bundle.
 *
 * @code{.ttl}
 * [] a ingen:Undo .
 * @endcode
 */
void
AtomWriter::operator()(const Undo& message)
{
	LV2_Atom_Forge_Frame msg;
	forge_request(&msg, _uris.ingen_Undo, message.seq);
	lv2_atom_forge_pop(&_forge, &msg);
	finish_msg();
}

/** @page protocol
 * @subsection Undo
 *
 * Use [ingen:Redo](http://drobilla.net/ns/ingen#Redo) to redo the last undone change.
 *
 * @code{.ttl}
 * [] a ingen:Redo .
 * @endcode
 */
void
AtomWriter::operator()(const Redo& message)
{
	LV2_Atom_Forge_Frame msg;
	forge_request(&msg, _uris.ingen_Redo, message.seq);
	lv2_atom_forge_pop(&_forge, &msg);
	finish_msg();
}

/** @page protocol
 * @subsection Get
 *
 * Send a [Get](http://lv2plug.in/ns/ext/patch#Get) to get the description
 * of the subject.
 *
 * @code{.ttl}
 * []
 *     a patch:Get ;
 *     patch:subject </main/osc> .
 * @endcode
 */
void
AtomWriter::operator()(const Get& message)
{
	LV2_Atom_Forge_Frame msg;
	forge_request(&msg, _uris.patch_Get, message.seq);
	lv2_atom_forge_key(&_forge, _uris.patch_subject);
	forge_uri(message.subject);
	lv2_atom_forge_pop(&_forge, &msg);
	finish_msg();
}

/** @page protocol
 *
 * @section arcs Arc Manipulation
 */

/** @page protocol
 * @subsection Connect Connecting Ports
 *
 * Ports are connected by putting an arc with the desired tail (an output port)
 * and head (an input port).  The tail and head must both be within the
 * subject, which must be a graph.
 *
 * @code{.ttl}
 * []
 *     a patch:Put ;
 *     patch:subject </main/> ;
 *     patch:body [
 *         a ingen:Arc ;
 *         ingen:tail </main/osc/out> ;
 *         ingen:head </main/filt/in> ;
 *     ] .
 * @endcode
 */
void
AtomWriter::operator()(const Connect& message)
{
	LV2_Atom_Forge_Frame msg;
	forge_request(&msg, _uris.patch_Put, message.seq);
	lv2_atom_forge_key(&_forge, _uris.patch_subject);
	forge_uri(path_to_uri(raul::Path::lca(message.tail, message.head)));
	lv2_atom_forge_key(&_forge, _uris.patch_body);
	forge_arc(message.tail, message.head);
	lv2_atom_forge_pop(&_forge, &msg);
	finish_msg();
}

/** @page protocol
 * @subsection Disconnect Disconnecting Ports
 *
 * Ports are disconnected by deleting the arc between them.  The description of
 * the arc is the same as in the put command used to create the connection.
 *
 * @code{.ttl}
 * []
 *     a patch:Delete ;
 *     patch:body [
 *         a ingen:Arc ;
 *         ingen:tail </main/osc/out> ;
 *         ingen:head </main/filt/in> ;
 *     ] .
 * @endcode
 */
void
AtomWriter::operator()(const Disconnect& message)
{
	LV2_Atom_Forge_Frame msg;
	forge_request(&msg, _uris.patch_Delete, message.seq);
	lv2_atom_forge_key(&_forge, _uris.patch_body);
	forge_arc(message.tail, message.head);
	lv2_atom_forge_pop(&_forge, &msg);
	finish_msg();
}

/** @page protocol
 * @subsection DisconnectAll Fully Disconnecting Anything
 *
 * All arcs to or from anything can be removed by giving the special property
 * ingen:incidentTo rather than a specific head and tail.  This works for both
 * ports and blocks (where the effect is to disconnect everything from ports on
 * that block).
 *
 * @code{.ttl}
 * []
 *     a patch:Delete ;
 *     patch:subject </main> ;
 *     patch:body [
 *         a ingen:Arc ;
 *         ingen:incidentTo </main/osc/out>
 *     ] .
 * @endcode
 */
void
AtomWriter::operator()(const DisconnectAll& message)
{
	LV2_Atom_Forge_Frame msg;
	forge_request(&msg, _uris.patch_Delete, message.seq);

	lv2_atom_forge_key(&_forge, _uris.patch_subject);
	forge_uri(path_to_uri(message.graph));

	lv2_atom_forge_key(&_forge, _uris.patch_body);
	LV2_Atom_Forge_Frame arc;
	lv2_atom_forge_object(&_forge, &arc, 0, _uris.ingen_Arc);
	lv2_atom_forge_key(&_forge, _uris.ingen_incidentTo);
	forge_uri(path_to_uri(message.path));
	lv2_atom_forge_pop(&_forge, &arc);

	lv2_atom_forge_pop(&_forge, &msg);
	finish_msg();
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
 *     patch:subject </main/osc> .
 * @endcode
 *
 * Might receive a response like:
 * @code{.ttl}
 * []
 *     a patch:Response ;
 *     patch:sequenceNumber 42 ;
 *     patch:subject </main/osc> ;
 *     patch:body 0 .
 * @endcode
 *
 * Where 0 is a status code, 0 meaning success and any other value being an
 * error.  Information about status codes, including error message strings,
 * are defined in ingen.lv2/errors.ttl.
 *
 * Note that a response is only a status response, operations that manipulate
 * the graph may generate new data on the stream, e.g. the above get request
 * would also receive a put that describes /main/osc in the stream immediately
 * following the response.
 */
void
AtomWriter::operator()(const Response& response)
{
	const auto& subject = response.subject;
	if (!response.id) {
		return;
	}

	LV2_Atom_Forge_Frame msg;
	forge_request(&msg, _uris.patch_Response, 0);
	lv2_atom_forge_key(&_forge, _uris.patch_sequenceNumber);
	lv2_atom_forge_int(&_forge, response.id);
	if (!subject.empty()) {
		lv2_atom_forge_key(&_forge, _uris.patch_subject);
		lv2_atom_forge_uri(&_forge, subject.c_str(), subject.length());
	}
	lv2_atom_forge_key(&_forge, _uris.patch_body);
	lv2_atom_forge_int(&_forge, static_cast<int>(response.status));
	lv2_atom_forge_pop(&_forge, &msg);
	finish_msg();
}

void
AtomWriter::operator()(const Error&)
{
}

/** @page protocol
 * @section loading Loading and Unloading Bundles
 *
 * The property ingen:loadedBundle on the engine can be used to load
 * or unload bundles from Ingen's world.  For example:
 *
 * @code{.ttl}
 * # Load /old.lv2
 * []
 *     a patch:Put ;
 *     patch:subject </> ;
 *     patch:body [
 *         ingen:loadedBundle <file:///old.lv2/>
 *     ] .
 *
 * # Replace /old.lv2 with /new.lv2
 * []
 *     a patch:Patch ;
 *     patch:subject </> ;
 *     patch:remove [
 *         ingen:loadedBundle <file:///old.lv2/>
 *     ];
 *     patch:add [
 *         ingen:loadedBundle <file:///new.lv2/>
 *     ] .
 * @endcode
 */

} // namespace ingen
