/* This file is part of Ingen.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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

#ifndef INGEN_CLIENT_PATCHMODEL_HPP
#define INGEN_CLIENT_PATCHMODEL_HPP

#include <cassert>
#include <sigc++/sigc++.h>
#include "raul/SharedPtr.hpp"
#include "interface/Patch.hpp"
#include "NodeModel.hpp"

#include "ConnectionModel.hpp"

namespace Ingen {

namespace Shared { class Port; }

namespace Client {

class ClientStore;


/** Client's model of a patch.
 *
 * \ingroup IngenClient
 */
class PatchModel : public NodeModel, public Ingen::Shared::Patch
{
public:
	/* WARNING: Copy constructor creates a shallow copy WRT connections */

	const Connections& connections() const { return *_connections.get(); }

	SharedPtr<ConnectionModel> get_connection(const Shared::Port* src_port,
	                                          const Shared::Port* dst_port);

	//uint32_t poly()          const { return _poly; }
	bool     enabled()       const;
	bool     polyphonic()    const;

	uint32_t internal_poly() const;

	/** "editable" = arranging,connecting,adding,deleting,etc
	 * not editable (control mode) you can just change controllers (performing)
	 */
	bool get_editable() const { return _editable; }
	void set_editable(bool e) {
		if (_editable != e) {
			_editable = e;
			signal_editable.emit(e);
		}
	}

	// Signals
	sigc::signal<void, SharedPtr<NodeModel> >       signal_new_node;
	sigc::signal<void, SharedPtr<NodeModel> >       signal_removed_node;
	sigc::signal<void, SharedPtr<ConnectionModel> > signal_new_connection;
	sigc::signal<void, SharedPtr<ConnectionModel> > signal_removed_connection;
	sigc::signal<void, bool>                        signal_editable;

private:
	friend class ClientStore;

	PatchModel(Shared::LV2URIMap& uris, const Raul::Path& patch_path)
		: NodeModel(uris, "http://drobilla.net/ns/ingen#Patch", patch_path)
		, _connections(new Connections())
		, _editable(true)
	{
	}

	void clear();
	void add_child(SharedPtr<ObjectModel> c);
	bool remove_child(SharedPtr<ObjectModel> c);

	void add_connection(SharedPtr<ConnectionModel> cm);
	void remove_connection(const Shared::Port* src_port, const Shared::Port* dst_port);

	SharedPtr<Connections> _connections;
	bool                   _editable;
};


} // namespace Client
} // namespace Ingen

#endif // INGEN_CLIENT_PATCHMODEL_HPP
