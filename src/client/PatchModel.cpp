/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
 *
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <cassert>
#include "raul/log.hpp"
#include "shared/LV2URIMap.hpp"
#include "PatchModel.hpp"
#include "NodeModel.hpp"
#include "ConnectionModel.hpp"
#include "ClientStore.hpp"

using namespace std;
using namespace Raul;

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
		signal_new_node.emit(nm);
}


bool
PatchModel::remove_child(SharedPtr<ObjectModel> o)
{
	assert(o->path().is_child_of(path()));
	assert(o->parent().get() == this);

	// Remove any connections which referred to this object,
	// since they can't possibly exist anymore
	for (Connections::iterator j = _connections->begin(); j != _connections->end(); ) {
		Connections::iterator next = j;
		++next;

		SharedPtr<ConnectionModel> cm = PtrCast<ConnectionModel>(j->second);
		assert(cm);

		if (cm->src_port_path().parent() == o->path()
				|| cm->src_port_path() == o->path()
				|| cm->dst_port_path().parent() == o->path()
				|| cm->dst_port_path() == o->path()) {
			signal_removed_connection.emit(cm);
			_connections->erase(j); // cuts our reference
		}
		j = next;
	}

	SharedPtr<PortModel> pm = PtrCast<PortModel>(o);
	if (pm)
		remove_port(pm);

	SharedPtr<NodeModel> nm = PtrCast<NodeModel>(o);
	if (nm)
		signal_removed_node.emit(nm);

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


SharedPtr<ConnectionModel>
PatchModel::get_connection(const Shared::Port* src_port, const Shared::Port* dst_port)
{
	Connections::iterator i = _connections->find(make_pair(src_port, dst_port));
	if (i != _connections->end())
		return PtrCast<ConnectionModel>(i->second);
	else
		return SharedPtr<ConnectionModel>();
}


/** Add a connection to this patch.
 *
 * A reference to  @a cm is taken, released on deletion or removal.
 * If @a cm only contains paths (not pointers to the actual ports), the ports
 * will be found and set.  The ports referred to not existing as children of
 * this patch is a fatal error.
 */
void
PatchModel::add_connection(SharedPtr<ConnectionModel> cm)
{
	// Store should have 'resolved' the connection already
	assert(cm);
	assert(cm->src_port());
	assert(cm->dst_port());
	assert(cm->src_port()->parent());
	assert(cm->dst_port()->parent());
	assert(cm->src_port_path() != cm->dst_port_path());
	assert(cm->src_port()->parent().get() == this
	       || cm->src_port()->parent()->parent().get() == this);
	assert(cm->dst_port()->parent().get() == this
	       || cm->dst_port()->parent()->parent().get() == this);

	SharedPtr<ConnectionModel> existing = get_connection(
			cm->src_port().get(), cm->dst_port().get());

	if (existing) {
		assert(cm->src_port() == existing->src_port());
		assert(cm->dst_port() == existing->dst_port());
	} else {
		_connections->insert(make_pair(make_pair(cm->src_port().get(), cm->dst_port().get()), cm));
		signal_new_connection.emit(cm);
	}
}


void
PatchModel::remove_connection(const Shared::Port* src_port, const Shared::Port* dst_port)
{
	Connections::iterator i = _connections->find(make_pair(src_port, dst_port));
	if (i != _connections->end()) {
		SharedPtr<ConnectionModel> c = PtrCast<ConnectionModel>(i->second);
		signal_removed_connection.emit(c);
		_connections->erase(i);
	} else {
		warn << "[PatchModel::remove_connection] Failed to find connection " <<
				src_port->path() << " -> " << dst_port->path() << endl;
	}
}


bool
PatchModel::enabled() const
{
	const Raul::Atom& enabled = get_property(Shared::LV2URIMap::instance().ingen_enabled);
	return (enabled.is_valid() && enabled.get_bool());
}


uint32_t
PatchModel::internal_poly() const
{
	const Raul::Atom& poly = get_property(Shared::LV2URIMap::instance().ingen_polyphony);
	return poly.is_valid() ? poly.get_int32() : 1;
}


bool
PatchModel::polyphonic() const
{
	const Raul::Atom& poly = get_property(Shared::LV2URIMap::instance().ingen_polyphonic);
	return poly.is_valid() && poly.get_bool();
}


} // namespace Client
} // namespace Ingen
