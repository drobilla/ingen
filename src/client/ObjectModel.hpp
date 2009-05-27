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

#ifndef OBJECTMODEL_H
#define OBJECTMODEL_H

#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <cassert>
#include <boost/utility.hpp>
#include <sigc++/sigc++.h>
#include "raul/Atom.hpp"
#include "raul/Path.hpp"
#include "raul/URI.hpp"
#include "raul/SharedPtr.hpp"
#include "interface/GraphObject.hpp"
#include "shared/ResourceImpl.hpp"

namespace Ingen {
namespace Client {

class ClientStore;


/** Base class for all GraphObject models (NodeModel, PatchModel, PortModel).
 *
 * There are no non-const public methods intentionally, models are not allowed
 * to be manipulated directly by anything (but the Store) because of the
 * asynchronous nature of engine control.  To change something, use the
 * controller (which the model probably shouldn't have a reference to but oh
 * well, it reduces Collection Hell) and wait for the result (as a signal
 * from this Model).
 *
 * \ingroup IngenClient
 */
class ObjectModel : virtual public Ingen::Shared::GraphObject
                  , public Ingen::Shared::ResourceImpl
{
public:
	virtual ~ObjectModel();

	const Raul::Atom& get_variable(const Raul::URI& key) const;
	Raul::Atom&       get_variable(Raul::URI& key);

	virtual void set_property(const Raul::URI& key, const Raul::Atom& value) {
		ResourceImpl::set_property(key, value);
		signal_property.emit(key, value);
	}

	virtual void set_meta_property(const Raul::URI& key, const Raul::Atom& value) {
		_meta.set_property(key, value);
		signal_property.emit(key, value);
	}

	Resource&              meta()             { return _meta; }
	const Resource&        meta()       const { return _meta; }
	const Properties&      variables()  const { return _variables; }
	Properties&            variables()        { return _variables; }
	const Raul::Path       path()       const { return _path; }
	const Raul::Symbol     symbol()     const { return _path.name(); }
	SharedPtr<ObjectModel> parent()     const { return _parent; }
	bool                   polyphonic() const;

	GraphObject* graph_parent() const { return _parent.get(); }

	// Signals
	sigc::signal<void, SharedPtr<ObjectModel> >             signal_new_child;
	sigc::signal<void, SharedPtr<ObjectModel> >             signal_removed_child;
	sigc::signal<void, const Raul::URI&, const Raul::Atom&> signal_property;
	sigc::signal<void>                                      signal_destroyed;
	sigc::signal<void>                                      signal_moved;

protected:
	friend class ClientStore;

	ObjectModel(const Raul::Path& path);

	virtual void set_path(const Raul::Path& p) { _path = p; signal_moved.emit(); }
	virtual void set_parent(SharedPtr<ObjectModel> p) { assert(p); _parent = p; }
	virtual void add_child(SharedPtr<ObjectModel> c) {}
	virtual bool remove_child(SharedPtr<ObjectModel> c) { return true; }

	virtual void set(SharedPtr<ObjectModel> model);

	ResourceImpl           _meta;
	Raul::Path             _path;
	SharedPtr<ObjectModel> _parent;

	Properties _variables;
};


} // namespace Client
} // namespace Ingen

#endif // OBJECTMODEL_H
