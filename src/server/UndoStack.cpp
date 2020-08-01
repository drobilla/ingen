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
#include "sratom/sratom.h"

#include <ctime>
#include <iterator>
#include <memory>

#define NS_RDF "http://www.w3.org/1999/02/22-rdf-syntax-ns#"

#define USTR(s) reinterpret_cast<const uint8_t*>(s)

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

	const auto* f = reinterpret_cast<const LV2_Atom_Object*>(first);
	const auto* s = reinterpret_cast<const LV2_Atom_Object*>(second);
	if (f->body.otype == _uris.patch_Set && f->body.otype == s->body.otype) {
		const LV2_Atom* f_subject  = nullptr;
		const LV2_Atom* f_property = nullptr;
		const LV2_Atom* s_subject  = nullptr;
		const LV2_Atom* s_property = nullptr;
		lv2_atom_object_get(f,
		                    _uris.patch_subject.urid(), &f_subject,
		                    _uris.patch_property.urid(), &f_property,
		                    0);
		lv2_atom_object_get(s,
		                    _uris.patch_subject.urid(), &s_subject,
		                    _uris.patch_property.urid(), &s_property,
		                    0);
		return (lv2_atom_equals(f_subject, s_subject) &&
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

	SerdNode get() {
		snprintf(buf, sizeof(buf), "%c%u", c, n++);
		return serd_node_from_string(SERD_BLANK, USTR(buf));
	}

	char       buf[16]{};
	unsigned   n{0};
	const char c{'b'};
};

struct ListContext {
	explicit ListContext(BlankIDs& ids, unsigned flags, const SerdNode* s, const SerdNode* p)
		: ids(ids)
		, s(*s)
		, p(*p)
		, flags(flags | SERD_LIST_O_BEGIN)
	{}

	SerdNode start_node(SerdWriter* writer) {
		const SerdNode node = ids.get();
		serd_writer_write_statement(writer, flags, nullptr, &s, &p, &node, nullptr, nullptr);
		return node;
	}

	void append(SerdWriter* writer, unsigned oflags, const SerdNode* value) {
		// s p node
		const SerdNode node = start_node(writer);

		// node rdf:first value
		p = serd_node_from_string(SERD_URI, USTR(NS_RDF "first"));
		flags = SERD_LIST_CONT;
		serd_writer_write_statement(writer, flags|oflags, nullptr, &node, &p, value, nullptr, nullptr);

		end_node(writer, &node);
	}

	void end_node(SerdWriter*, const SerdNode* node) {
		// Prepare for next call: node rdf:rest ...
		s = *node;
		p = serd_node_from_string(SERD_URI, USTR(NS_RDF "rest"));
	}

	void end(SerdWriter* writer) {
		const SerdNode nil =
		    serd_node_from_string(SERD_URI, USTR(NS_RDF "nil"));

		serd_writer_write_statement(
		    writer, flags, nullptr, &s, &p, &nil, nullptr, nullptr);
	}

	BlankIDs& ids;
	SerdNode  s;
	SerdNode  p;
	unsigned  flags;
};

void
UndoStack::write_entry(Sratom*                 sratom,
                       SerdWriter*             writer,
                       const SerdNode* const   subject,
                       const UndoStack::Entry& entry)
{
	char time_str[24];
	strftime(time_str, sizeof(time_str), "%FT%T", gmtime(&entry.time));

	// entry rdf:type ingen:UndoEntry
	SerdNode p = serd_node_from_string(SERD_URI, USTR(INGEN_NS "time"));
	SerdNode o = serd_node_from_string(SERD_LITERAL, USTR(time_str));
	serd_writer_write_statement(writer, SERD_ANON_CONT, nullptr, subject, &p, &o, nullptr, nullptr);

	p = serd_node_from_string(SERD_URI, USTR(INGEN_NS "events"));

	BlankIDs    ids('e');
	ListContext ctx(ids, SERD_ANON_CONT, subject, &p);

	for (const LV2_Atom* atom : entry.events) {
		const SerdNode node = ctx.start_node(writer);

		p = serd_node_from_string(SERD_URI,
		                          reinterpret_cast<const uint8_t*>(NS_RDF
		                                                           "first"));

		ctx.flags = SERD_LIST_CONT;
		sratom_write(sratom, &_map.urid_unmap_feature()->urid_unmap, SERD_LIST_CONT,
		             &node, &p,
		             atom->type, atom->size, LV2_ATOM_BODY_CONST(atom));

		ctx.end_node(writer, &node);
	}

	ctx.end(writer);
}

void
UndoStack::save(FILE* stream, const char* name)
{
	SerdEnv* env = serd_env_new(nullptr);
	serd_env_set_prefix_from_strings(env, USTR("atom"),  USTR(LV2_ATOM_PREFIX));
	serd_env_set_prefix_from_strings(env, USTR("ingen"), USTR(INGEN_NS));
	serd_env_set_prefix_from_strings(env, USTR("patch"), USTR(LV2_PATCH_PREFIX));

	const SerdNode base = serd_node_from_string(SERD_URI, USTR("ingen:/"));
	SerdURI        base_uri;
	serd_uri_parse(base.buf, &base_uri);

	SerdWriter* writer =
	    serd_writer_new(SERD_TURTLE,
	                    static_cast<SerdStyle>(SERD_STYLE_RESOLVED |
	                                           SERD_STYLE_ABBREVIATED |
	                                           SERD_STYLE_CURIED),
	                    env,
	                    &base_uri,
	                    serd_file_sink,
	                    stream);

	// Configure sratom to write directly to the writer (and thus the socket)
	Sratom* sratom = sratom_new(&_map.urid_map_feature()->urid_map);
	sratom_set_sink(sratom,
	                reinterpret_cast<const char*>(base.buf),
	                reinterpret_cast<SerdStatementSink>(serd_writer_write_statement),
	                reinterpret_cast<SerdEndSink>(serd_writer_end_anon),
	                writer);

	SerdNode s = serd_node_from_string(SERD_BLANK, USTR(name));
	SerdNode p = serd_node_from_string(SERD_URI, USTR(INGEN_NS "entries"));

	BlankIDs    ids('u');
	ListContext ctx(ids, 0, &s, &p);
	for (const Entry& e : _stack) {
		const SerdNode entry = ids.get();
		ctx.append(writer, SERD_ANON_O_BEGIN, &entry);
		write_entry(sratom, writer, &entry, e);
		serd_writer_end_anon(writer, &entry);
	}
	ctx.end(writer);

	sratom_free(sratom);
	serd_writer_finish(writer);
	serd_writer_free(writer);
}

} // namespace server
} // namespace ingen
