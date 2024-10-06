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

#include "ingen/client/GraphModel.hpp"

#include "ingen/Atom.hpp"
#include "ingen/URI.hpp"
#include "ingen/URIs.hpp"
#include "ingen/client/ArcModel.hpp"
#include "ingen/client/BlockModel.hpp"
#include "ingen/client/ObjectModel.hpp"
#include "ingen/client/PortModel.hpp"
#include "raul/Path.hpp"

#include <cassert>
#include <map>
#include <memory>
#include <string>
#include <utility>

namespace ingen::client {

GraphModel::GraphModel(URIs& uris, const raul::Path& graph_path)
    : BlockModel{uris, static_cast<const URI&>(uris.ingen_Graph), graph_path}
{
}

void
GraphModel::add_child(const std::shared_ptr<ObjectModel>& c)
{
	assert(c->parent().get() == this);

	auto pm = std::dynamic_pointer_cast<PortModel>(c);
	if (pm) {
		add_port(pm);
		return;
	}

	auto bm = std::dynamic_pointer_cast<BlockModel>(c);
	if (bm) {
		_signal_new_block.emit(bm);
	}
}

bool
GraphModel::remove_child(const std::shared_ptr<ObjectModel>& o)
{
	assert(o->path().is_child_of(path()));
	assert(o->parent().get() == this);

	auto pm = std::dynamic_pointer_cast<PortModel>(o);
	if (pm) {
		remove_arcs_on(pm);
		remove_port(pm);
	}

	auto bm = std::dynamic_pointer_cast<BlockModel>(o);
	if (bm) {
		_signal_removed_block.emit(bm);
	}

	return true;
}

void
GraphModel::remove_arcs_on(const std::shared_ptr<PortModel>& p)
{
	// Remove any connections which referred to this object,
	// since they can't possibly exist anymore
	for (auto j = _graph_arcs.begin(); j != _graph_arcs.end();) {
		auto next = j;
		++next;

		auto arc = std::dynamic_pointer_cast<ArcModel>(j->second);
		if (arc->tail_path().parent() == p->path()
		    || arc->tail_path() == p->path()
		    || arc->head_path().parent() == p->path()
		    || arc->head_path() == p->path()) {
			_signal_removed_arc.emit(arc);
			_graph_arcs.erase(j); // Cuts our reference
		}
		j = next;
	}
}

void
GraphModel::clear()
{
	_graph_arcs.clear();

	BlockModel::clear();

	assert(_graph_arcs.empty());
	assert(_ports.empty());
}

std::shared_ptr<ArcModel>
GraphModel::get_arc(const Node* tail, const Node* head)
{
	auto i = _graph_arcs.find(std::make_pair(tail, head));
	if (i != _graph_arcs.end()) {
		return std::dynamic_pointer_cast<ArcModel>(i->second);
	}

	return nullptr;
}

/** Add a connection to this graph.
 *
 * A reference to `arc` is taken, released on deletion or removal.
 * If `arc` only contains paths (not pointers to the actual ports), the ports
 * will be found and set.  The ports referred to not existing as children of
 * this graph is a fatal error.
 */
void
GraphModel::add_arc(const std::shared_ptr<ArcModel>& arc)
{
	// Store should have 'resolved' the connection already
	assert(arc);
	assert(arc->tail());
	assert(arc->head());
	assert(arc->tail()->parent());
	assert(arc->head()->parent());
	assert(arc->tail_path() != arc->head_path());
	assert(arc->tail()->parent().get() == this
	       || arc->tail()->parent()->parent().get() == this);
	assert(arc->head()->parent().get() == this
	       || arc->head()->parent()->parent().get() == this);

	const std::shared_ptr<ArcModel> existing = get_arc(
		arc->tail().get(), arc->head().get());

	if (existing) {
		assert(arc->tail() == existing->tail());
		assert(arc->head() == existing->head());
	} else {
		_graph_arcs.emplace(std::make_pair(arc->tail().get(),
		                                   arc->head().get()),
		                    arc);
		_signal_new_arc.emit(arc);
	}
}

void
GraphModel::remove_arc(const Node* tail, const Node* head)
{
	auto i = _graph_arcs.find(std::make_pair(tail, head));
	if (i != _graph_arcs.end()) {
		auto arc = std::dynamic_pointer_cast<ArcModel>(i->second);
		_signal_removed_arc.emit(arc);
		_graph_arcs.erase(i);
	}
}

bool
GraphModel::enabled() const
{
	const Atom& enabled = get_property(_uris.ingen_enabled);
	return (enabled.is_valid() && enabled.get<int32_t>());
}

uint32_t
GraphModel::internal_poly() const
{
	const Atom& poly = get_property(_uris.ingen_polyphony);
	return poly.is_valid() ? poly.get<int32_t>() : 1;
}

} // namespace ingen::client
