/*
  This file is part of Ingen.
  Copyright 2016 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "UndoStack.hpp"

#include "ingen/URIMap.hpp"
#include "ingen/URIs.hpp"
#include "lv2/atom/atom.h"
#include "lv2/atom/util.h"
#include "lv2/patch/patch.h"
#include "lv2/urid/urid.h"
#include "serd/serd.h"
#include "sratom/sratom.hpp"

#include <ctime>
#include <fstream>
#include <iterator>
#include <memory>

#define NS_RDF "http://www.w3.org/1999/02/22-rdf-syntax-ns#"

namespace ingen {
namespace server {

int
UndoStack::start_entry()
{
	if (_depth == 0) {
		time_t now;
		time(&now);
		_stack.emplace_back(Entry(now));
	}
	return ++_depth;
}

bool
UndoStack::write(const LV2_Atom* msg, int32_t)
{
	_stack.back().push_event(msg);
	return true;
}

bool
UndoStack::ignore_later_event(const LV2_Atom* first,
                              const LV2_Atom* second) const
{
	if (first->type != _uris.atom_Object || first->type != second->type) {
		return false;
	}

	const auto* f = (const LV2_Atom_Object*)first;
	const auto* s = (const LV2_Atom_Object*)second;
	if (f->body.otype == _uris.patch_Set && f->body.otype == s->body.otype) {
		const LV2_Atom* f_subject  = nullptr;
		const LV2_Atom* f_property = nullptr;
		const LV2_Atom* s_subject  = nullptr;
		const LV2_Atom* s_property = nullptr;
		lv2_atom_object_get(f,
		                    (LV2_URID)_uris.patch_subject,  &f_subject,
		                    (LV2_URID)_uris.patch_property, &f_property,
		                    0);
		lv2_atom_object_get(s,
		                    (LV2_URID)_uris.patch_subject,  &s_subject,
		                    (LV2_URID)_uris.patch_property, &s_property,
		                    0);
		return (lv2_atom_equals(f_subject,  s_subject) &&
		        lv2_atom_equals(f_property, s_property));
	}

	return false;
}

int
UndoStack::finish_entry()
{
	if (--_depth > 0) {
		return _depth;
	} else if (_stack.back().events.empty()) {
		// Disregard empty entry
		_stack.pop_back();
	} else if (_stack.size() > 1 && _stack.back().events.size() == 1) {
		// This entry and the previous one have one event, attempt to merge
		auto i = _stack.rbegin();
		++i;
		if (i->events.size() == 1) {
			if (ignore_later_event(i->events[0], _stack.back().events[0])) {
				_stack.pop_back();
			}
		}
	}

	return _depth;
}

UndoStack::Entry
UndoStack::pop()
{
	Entry top;
	if (!_stack.empty()) {
		top = _stack.back();
		_stack.pop_back();
	}
	return top;
}

struct BlankIDs {
	explicit BlankIDs(char c='b') : c(c) {}

	serd::Node get() {
		snprintf(buf, sizeof(buf), "%c%u", c, n++);
		return serd::make_blank(buf);
	}

	char       buf[16]{};
	unsigned   n{0};
	const char c{'b'};
};

struct ListContext {
	explicit ListContext(BlankIDs&            ids,
	                     serd::StatementFlags flags,
	                     const serd::Node&    s,
	                     const serd::Node&    p)
	    : ids(ids)
	    , s(s)
	    , p(p)
	    , flags(flags | serd::StatementFlag::list_O)
	{}

	serd::Node start_node(serd::Writer& writer) {
		const serd::Node node = ids.get();
		writer.sink().write(flags, s, p, node);
		return node;
	}

	void append(serd::Writer&        writer,
	            serd::StatementFlags oflags,
	            const serd::Node&    value)
	{
		// s p node
		const serd::Node node = start_node(writer);

		// node rdf:first value
		p     = serd::make_uri(NS_RDF "first");
		flags = {};
		writer.sink().write(flags | oflags, node, p, value);

		end_node(writer, node);
	}

	void end_node(serd::Writer&, const serd::Node& node) {
		// Prepare for next call: node rdf:rest ...
		s = node;
		p = serd::make_uri(NS_RDF "rest");
	}

	void end(serd::Writer& writer) {
		const serd::Node nil = serd::make_uri(NS_RDF "nil");
		writer.sink().write(flags, s, p, nil);
	}

	BlankIDs&            ids;
	serd::Node           s;
	serd::Node           p;
	serd::StatementFlags flags;
};

void
UndoStack::write_entry(sratom::Streamer&       streamer,
                       serd::Writer&           writer,
                       const serd::Node&       subject,
                       const UndoStack::Entry& entry)
{
	char time_str[24];
	strftime(time_str, sizeof(time_str), "%FT%T", gmtime(&entry.time));

	writer.sink().write({},
	                    subject,
	                    serd::make_uri(INGEN_NS "time"),
	                    serd::make_string(time_str));

	serd::Node p = serd::make_uri(INGEN_NS "events");

	BlankIDs    ids('e');
	ListContext ctx(ids, {}, subject, p);

	for (const LV2_Atom* atom : entry.events) {
		const serd::Node node = ctx.start_node(writer);

		p         = serd::make_uri(NS_RDF "first");
		ctx.flags = {};
		streamer.write(writer.sink(), node, p, *atom);
		ctx.end_node(writer, node);
	}

	ctx.end(writer);
}

void
UndoStack::save(std::ofstream& stream, const char* name)
{
	serd::Env env;
	env.set_prefix("atom",  LV2_ATOM_PREFIX);
	env.set_prefix("ingen", INGEN_NS);
	env.set_prefix("patch", LV2_PATCH_PREFIX);

	const serd::Node base = serd::make_uri("ingen:/");

	serd::Writer writer(_world,
	                    serd::Syntax::Turtle,
	                    {},
	                    env,
	                    stream);

	// Configure sratom to write directly to the writer (and thus the socket)
	sratom::Streamer streamer{_world,
	                          _map.urid_map_feature()->urid_map,
	                          _map.urid_unmap_feature()->urid_unmap};

	serd::Node s = serd::make_blank(name);
	serd::Node p = serd::make_uri(INGEN_NS "entries");

	BlankIDs    ids('u');
	ListContext ctx(ids, {}, s, p);
	for (const Entry& e : _stack) {
		const serd::Node entry = ids.get();
		ctx.append(writer, serd::StatementFlag::anon_O, entry);
		write_entry(streamer, writer, entry, e);
		writer.sink().end(entry);
	}
	ctx.end(writer);

	writer.finish();
}

} // namespace server
} // namespace ingen
