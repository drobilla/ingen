/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
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

#ifndef INGEN_CLIENT_OBJECTMODEL_HPP
#define INGEN_CLIENT_OBJECTMODEL_HPP

#include <algorithm>
#include <cassert>
#include <cstdlib>

#include "raul/Path.hpp"
#include "raul/SharedPtr.hpp"
#include "raul/URI.hpp"

#include "ingen/GraphObject.hpp"
#include "ingen/client/signal.hpp"
#include "shared/ResourceImpl.hpp"

namespace Ingen {

namespace Shared { class LV2URIMap; }

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
class ObjectModel : virtual public GraphObject
                  , public Ingen::Shared::ResourceImpl
{
public:
	virtual ~ObjectModel();

	const Raul::Atom& get_property(const Raul::URI& key) const;

	Raul::Atom& set_property(const Raul::URI& key, const Raul::Atom& value,
	                         Resource::Graph ctx);
	void        add_property(const Raul::URI& key, const Raul::Atom& value);

	const Raul::Path&      path()       const { return _path; }
	const Raul::Symbol&    symbol()     const { return _symbol; }
	SharedPtr<ObjectModel> parent()     const { return _parent; }
	bool                   polyphonic() const;

	GraphObject* graph_parent() const { return _parent.get(); }

	// Signals
	INGEN_SIGNAL(new_child, void, SharedPtr<ObjectModel>);
	INGEN_SIGNAL(removed_child, void, SharedPtr<ObjectModel>);
	INGEN_SIGNAL(property, void, const Raul::URI&, const Raul::Atom&);
	INGEN_SIGNAL(destroyed, void);
	INGEN_SIGNAL(moved, void);

protected:
	friend class ClientStore;

	ObjectModel(Shared::LV2URIMap& uris, const Raul::Path& path);
	ObjectModel(const ObjectModel& copy);

	virtual void set_path(const Raul::Path& p);
	virtual void set_parent(SharedPtr<ObjectModel> p);
	virtual void add_child(SharedPtr<ObjectModel> c) {}
	virtual bool remove_child(SharedPtr<ObjectModel> c) { return true; }

	virtual void set(SharedPtr<ObjectModel> model);

	ResourceImpl           _meta;
	SharedPtr<ObjectModel> _parent;

private:
	Raul::Path   _path;
	Raul::Symbol _symbol;
};

} // namespace Client
} // namespace Ingen

#endif // INGEN_CLIENT_OBJECTMODEL_HPP
