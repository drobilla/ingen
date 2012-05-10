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

#include "raul/log.hpp"

#include "ingen/client/ClientStore.hpp"
#include "ingen/client/EdgeModel.hpp"
#include "ingen/client/NodeModel.hpp"
#include "ingen/client/PatchModel.hpp"
#include "ingen/shared/URIs.hpp"

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

	SharedPtr<NodeModel> nm = PtrCast<NodeModel>(c);
	if (nm)
		_signal_new_node.emit(nm);
}

bool
PatchModel::remove_child(SharedPtr<ObjectModel> o)
{
	assert(o->path().is_child_of(path()));
	assert(o->parent().get() == this);

	// Remove any connections which referred to this object,
	// since they can't possibly exist anymore
	for (Connections::iterator j = _connections->begin();
	     j != _connections->end();) {
		Connections::iterator next = j;
		++next;

		SharedPtr<EdgeModel> cm = PtrCast<EdgeModel>(j->second);
		assert(cm);

		if (cm->tail_path().parent() == o->path()
				|| cm->tail_path() == o->path()
				|| cm->head_path().parent() == o->path()
				|| cm->head_path() == o->path()) {
			_signal_removed_connection.emit(cm);
			_connections->erase(j); // cuts our reference
		}
		j = next;
	}

	SharedPtr<PortModel> pm = PtrCast<PortModel>(o);
	if (pm)
		remove_port(pm);

	SharedPtr<NodeModel> nm = PtrCast<NodeModel>(o);
	if (nm)
		_signal_removed_node.emit(nm);

	return true;
}

void
PatchModel::clear()
{
	_connections->clear();

	NodeModel::clear();

	assert(_connections->empty());
	assert(_ports.empty());
}

SharedPtr<EdgeModel>
PatchModel::get_connection(const Port* tail, const Ingen::Port* head)
{
	Connections::iterator i = _connections->find(make_pair(tail, head));
	if (i != _connections->end())
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
PatchModel::add_connection(SharedPtr<EdgeModel> cm)
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

	SharedPtr<EdgeModel> existing = get_connection(
			cm->tail().get(), cm->head().get());

	if (existing) {
		assert(cm->tail() == existing->tail());
		assert(cm->head() == existing->head());
	} else {
		_connections->insert(make_pair(make_pair(cm->tail().get(),
		                                         cm->head().get()), cm));
		_signal_new_connection.emit(cm);
	}
}

void
PatchModel::remove_connection(const Port* tail, const Ingen::Port* head)
{
	Connections::iterator i = _connections->find(make_pair(tail, head));
	if (i != _connections->end()) {
		SharedPtr<EdgeModel> c = PtrCast<EdgeModel>(i->second);
		_signal_removed_connection.emit(c);
		_connections->erase(i);
	} else {
		Raul::warn(Raul::fmt("Failed to remove patch connection %1% => %2%\n")
		           % tail->path() % head->path());
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
