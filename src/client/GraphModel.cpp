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

#include <cassert>

#include "ingen/URIs.hpp"
#include "ingen/client/ArcModel.hpp"
#include "ingen/client/BlockModel.hpp"
#include "ingen/client/ClientStore.hpp"
#include "ingen/client/GraphModel.hpp"

using namespace std;

namespace Ingen {
namespace Client {

void
GraphModel::add_child(SPtr<ObjectModel> c)
{
	assert(c->parent().get() == this);

	SPtr<PortModel> pm = dynamic_ptr_cast<PortModel>(c);
	if (pm) {
		add_port(pm);
		return;
	}

	SPtr<BlockModel> bm = dynamic_ptr_cast<BlockModel>(c);
	if (bm) {
		_signal_new_block.emit(bm);
	}
}

bool
GraphModel::remove_child(SPtr<ObjectModel> o)
{
	assert(o->path().is_child_of(path()));
	assert(o->parent().get() == this);

	// Remove any connections which referred to this object,
	// since they can't possibly exist anymore
	for (Arcs::iterator j = _arcs.begin(); j != _arcs.end();) {
		Arcs::iterator next = j;
		++next;

		SPtr<ArcModel> arc = dynamic_ptr_cast<ArcModel>(j->second);
		if (arc->tail_path().parent() == o->path()
				|| arc->tail_path() == o->path()
				|| arc->head_path().parent() == o->path()
				|| arc->head_path() == o->path()) {
			_signal_removed_arc.emit(arc);
			_arcs.erase(j); // cuts our reference
		}
		j = next;
	}

	SPtr<PortModel> pm = dynamic_ptr_cast<PortModel>(o);
	if (pm)
		remove_port(pm);

	SPtr<BlockModel> bm = dynamic_ptr_cast<BlockModel>(o);
	if (bm) {
		_signal_removed_block.emit(bm);
	}

	return true;
}

void
GraphModel::clear()
{
	_arcs.clear();

	BlockModel::clear();

	assert(_arcs.empty());
	assert(_ports.empty());
}

SPtr<ArcModel>
GraphModel::get_arc(const Node* tail, const Node* head)
{
	Arcs::iterator i = _arcs.find(make_pair(tail, head));
	if (i != _arcs.end())
		return dynamic_ptr_cast<ArcModel>(i->second);
	else
		return SPtr<ArcModel>();
}

/** Add a connection to this graph.
 *
 * A reference to @p arc is taken, released on deletion or removal.
 * If @p arc only contains paths (not pointers to the actual ports), the ports
 * will be found and set.  The ports referred to not existing as children of
 * this graph is a fatal error.
 */
void
GraphModel::add_arc(SPtr<ArcModel> arc)
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

	SPtr<ArcModel> existing = get_arc(
			arc->tail().get(), arc->head().get());

	if (existing) {
		assert(arc->tail() == existing->tail());
		assert(arc->head() == existing->head());
	} else {
		_arcs.insert(make_pair(make_pair(arc->tail().get(),
		                                 arc->head().get()),
		                       arc));
		_signal_new_arc.emit(arc);
	}
}

void
GraphModel::remove_arc(const Node* tail, const Node* head)
{
	Arcs::iterator i = _arcs.find(make_pair(tail, head));
	if (i != _arcs.end()) {
		SPtr<ArcModel> arc = dynamic_ptr_cast<ArcModel>(i->second);
		_signal_removed_arc.emit(arc);
		_arcs.erase(i);
	}
}

bool
GraphModel::enabled() const
{
	const Raul::Atom& enabled = get_property(_uris.ingen_enabled);
	return (enabled.is_valid() && enabled.get_bool());
}

uint32_t
GraphModel::internal_poly() const
{
	const Raul::Atom& poly = get_property(_uris.ingen_polyphony);
	return poly.is_valid() ? poly.get_int32() : 1;
}

bool
GraphModel::polyphonic() const
{
	const Raul::Atom& poly = get_property(_uris.ingen_polyphonic);
	return poly.is_valid() && poly.get_bool();
}

} // namespace Client
} // namespace Ingen
