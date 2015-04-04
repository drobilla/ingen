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

/**
   @defgroup IngenClient Client-Side Models and Utilities
*/

#ifndef INGEN_CLIENT_OBJECTMODEL_HPP
#define INGEN_CLIENT_OBJECTMODEL_HPP

#include <algorithm>
#include <cassert>
#include <cstdlib>

#include "ingen/types.hpp"
#include "raul/Path.hpp"
#include "raul/URI.hpp"

#include "ingen/Node.hpp"
#include "ingen/Resource.hpp"
#include "ingen/client/signal.hpp"
#include "ingen/ingen.h"

namespace Ingen {

class URIs;

namespace Client {

class ClientStore;

/** Base class for all Node models (BlockModel, GraphModel, PortModel).
 *
 * There are no non-const public methods intentionally, models are not allowed
 * to be manipulated directly by anything (but the Store) because of the
 * asynchronous nature of engine control.  To change something, use the
 * controller (which the model probably shouldn't have a reference to but oh
 * well, it reduces Collection Hell) and wait for the result (as a signal
 * from this Model).
 *
 * @ingroup IngenClient
 */
class INGEN_API ObjectModel : public Node
{
public:
	virtual ~ObjectModel();

	bool is_a(const Raul::URI& type) const;

	const Atom& get_property(const Raul::URI& key) const;

	void on_property(const Raul::URI& uri, const Atom& value);
	void on_property_removed(const Raul::URI& uri, const Atom& value);

	const Raul::Path&   path()       const { return _path; }
	const Raul::Symbol& symbol()     const { return _symbol; }
	SPtr<ObjectModel>   parent()     const { return _parent; }
	bool                polyphonic() const;

	Node* graph_parent() const { return _parent.get(); }

	// Signals
	INGEN_SIGNAL(new_child, void, SPtr<ObjectModel>);
	INGEN_SIGNAL(removed_child, void, SPtr<ObjectModel>);
	INGEN_SIGNAL(property, void, const Raul::URI&, const Atom&);
	INGEN_SIGNAL(property_removed, void, const Raul::URI&, const Atom&);
	INGEN_SIGNAL(destroyed, void);
	INGEN_SIGNAL(moved, void);

protected:
	friend class ClientStore;

	ObjectModel(URIs& uris, const Raul::Path& path);
	ObjectModel(const ObjectModel& copy);

	virtual void set_path(const Raul::Path& p);
	virtual void set_parent(SPtr<ObjectModel> p);
	virtual void add_child(SPtr<ObjectModel> c) {}
	virtual bool remove_child(SPtr<ObjectModel> c) { return true; }

	virtual void set(SPtr<ObjectModel> model);

	SPtr<ObjectModel> _parent;

private:
	Raul::Path   _path;
	Raul::Symbol _symbol;
};

} // namespace Client
} // namespace Ingen

#endif // INGEN_CLIENT_OBJECTMODEL_HPP
