/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#ifndef PATCHMODEL_H
#define PATCHMODEL_H

#include <cassert>
#include <list>
#include <string>
#include <sigc++/sigc++.h>
#include <raul/SharedPtr.hpp>
#include "interface/Patch.hpp"
#include "NodeModel.hpp"

#include "ConnectionModel.hpp"

using std::list; using std::string;

namespace Ingen {
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
	
	SharedPtr<ConnectionModel> get_connection(const string& src_port_path,
	                                          const string& dst_port_path) const;
	
	uint32_t poly()               const { return _poly; }
	uint32_t internal_polyphony() const { return _poly; }
	bool     enabled()            const;
	bool     polyphonic()         const;

	/** "editable" = arranging,connecting,adding,deleting,etc
	 * not editable (control mode) you can just change controllers (performing)
	 */
	bool get_editable() const { return _editable; }
	void set_editable(bool e) { if (_editable != e) {
		_editable = e;
		signal_editable.emit(e);
	} }
	
	virtual void set_property(const string& key, const Atom& value);

	// Signals
	sigc::signal<void, SharedPtr<NodeModel> >       signal_new_node; 
	sigc::signal<void, SharedPtr<NodeModel> >       signal_removed_node; 
	sigc::signal<void, SharedPtr<ConnectionModel> > signal_new_connection; 
	sigc::signal<void, SharedPtr<ConnectionModel> > signal_removed_connection;
	sigc::signal<void, bool>                        signal_editable;

private:
	friend class ClientStore;

	PatchModel(const Path& patch_path, size_t internal_poly)
		: NodeModel("ingen:Patch", patch_path)
		, _connections(new Connections())
		, _poly(internal_poly)
		, _editable(true)
	{
	}
	
	void clear();
	void add_child(SharedPtr<ObjectModel> c);
	bool remove_child(SharedPtr<ObjectModel> c);
	
	void add_connection(SharedPtr<ConnectionModel> cm);
	void remove_connection(const string& src_port_path, const string& dst_port_path);
	
	SharedPtr<Connections> _connections;
	uint32_t               _poly;
	bool                   _editable;
};

typedef Raul::Table<string, SharedPtr<PatchModel> > PatchModelMap;


} // namespace Client
} // namespace Ingen

#endif // PATCHMODEL_H
