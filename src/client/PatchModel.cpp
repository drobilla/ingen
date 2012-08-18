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

#include "ingen/client/ClientStore.hpp"
#include "ingen/client/EdgeModel.hpp"
#include "ingen/client/BlockModel.hpp"
#include "ingen/client/PatchModel.hpp"
#include "ingen/URIs.hpp"

using namespace std;

namespace Ingen {
namespace Client {

void
PatchModel::add_child(SharedPtr<ObjectModel> c)
{
	assert(c->parent().get() == this);

	SharedPtr<PortModel> pm = PtrCast<PortModel>(c);
	if (pm) {
		add_port(pm);
		return;
	}

	SharedPtr<BlockModel> bm = PtrCast<BlockModel>(c);
	if (bm) {
		_signal_new_block.emit(bm);
	}
}

bool
PatchModel::remove_child(SharedPtr<ObjectModel> o)
{
	assert(o->path().is_child_of(path()));
	assert(o->parent().get() == this);

	// Remove any connections which referred to this object,
	// since they can't possibly exist anymore
	for (Edges::iterator j = _edges.begin(); j != _edges.end();) {
		Edges::iterator next = j;
		++next;

		SharedPtr<EdgeModel> cm = PtrCast<EdgeModel>(j->second);
		assert(cm);

		if (cm->tail_path().parent() == o->path()
				|| cm->tail_path() == o->path()
				|| cm->head_path().parent() == o->path()
				|| cm->head_path() == o->path()) {
			_signal_removed_edge.emit(cm);
			_edges.erase(j); // cuts our reference
		}
		j = next;
	}

	SharedPtr<PortModel> pm = PtrCast<PortModel>(o);
	if (pm)
		remove_port(pm);

	SharedPtr<BlockModel> bm = PtrCast<BlockModel>(o);
	if (bm) {
		_signal_removed_block.emit(bm);
	}

	return true;
}

void
PatchModel::clear()
{
	_edges.clear();

	BlockModel::clear();

	assert(_edges.empty());
	assert(_ports.empty());
}

SharedPtr<EdgeModel>
PatchModel::get_edge(const GraphObject* tail, const GraphObject* head)
{
	Edges::iterator i = _edges.find(make_pair(tail, head));
	if (i != _edges.end())
		return PtrCast<EdgeModel>(i->second);
	else
		return SharedPtr<EdgeModel>();
}

/** Add a connection to this patch.
 *
 * A reference to  @a cm is taken, released on deletion or removal.
 * If @a cm only contains paths (not pointers to the actual ports), the ports
 * will be found and set.  The ports referred to not existing as children of
 * this patch is a fatal error.
 */
void
PatchModel::add_edge(SharedPtr<EdgeModel> cm)
{
	// Store should have 'resolved' the connection already
	assert(cm);
	assert(cm->tail());
	assert(cm->head());
	assert(cm->tail()->parent());
	assert(cm->head()->parent());
	assert(cm->tail_path() != cm->head_path());
	assert(cm->tail()->parent().get() == this
	       || cm->tail()->parent()->parent().get() == this);
	assert(cm->head()->parent().get() == this
	       || cm->head()->parent()->parent().get() == this);

	SharedPtr<EdgeModel> existing = get_edge(
			cm->tail().get(), cm->head().get());

	if (existing) {
		assert(cm->tail() == existing->tail());
		assert(cm->head() == existing->head());
	} else {
		_edges.insert(make_pair(make_pair(cm->tail().get(),
		                                         cm->head().get()), cm));
		_signal_new_edge.emit(cm);
	}
}

void
PatchModel::remove_edge(const GraphObject* tail, const GraphObject* head)
{
	Edges::iterator i = _edges.find(make_pair(tail, head));
	if (i != _edges.end()) {
		SharedPtr<EdgeModel> c = PtrCast<EdgeModel>(i->second);
		_signal_removed_edge.emit(c);
		_edges.erase(i);
	}
}

bool
PatchModel::enabled() const
{
	const Raul::Atom& enabled = get_property(_uris.ingen_enabled);
	return (enabled.is_valid() && enabled.get_bool());
}

uint32_t
PatchModel::internal_poly() const
{
	const Raul::Atom& poly = get_property(_uris.ingen_polyphony);
	return poly.is_valid() ? poly.get_int32() : 1;
}

bool
PatchModel::polyphonic() const
{
	const Raul::Atom& poly = get_property(_uris.ingen_polyphonic);
	return poly.is_valid() && poly.get_bool();
}

} // namespace Client
} // namespace Ingen
