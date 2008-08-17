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

#ifndef OBJECTMODEL_H
#define OBJECTMODEL_H

#include <cstdlib>
#include <iostream>
#include <string>
#include <algorithm>
#include <cassert>
#include <boost/utility.hpp>
#include <sigc++/sigc++.h>
#include <raul/Atom.hpp>
#include <raul/Path.hpp>
#include <raul/SharedPtr.hpp>
#include <raul/PathTable.hpp>
#include "interface/GraphObject.hpp"

using Raul::PathTable;
using std::string;
using Raul::Atom;
using Raul::Path;
using Raul::Symbol;

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
{
public:
	virtual ~ObjectModel();

	const Atom& get_variable(const string& key) const;
	const Atom& get_property(const string& key) const;
	
	virtual void set_variable(const string& key, const Atom& value)
		{ _variables[key] = value; signal_variable.emit(key, value); }
	
	virtual void set_property(const string& key, const Atom& value)
		{ _properties[key] = value; signal_property.emit(key, value); }

	const Variables&       variables()  const { return _variables; }
	const Properties&      properties() const { return _properties; }
	const Path             path()       const { return _path; }
	const Symbol           symbol()     const { return _path.name(); }
	SharedPtr<ObjectModel> parent()     const { return _parent; }
	bool                   polyphonic() const;
	
	GraphObject* graph_parent() const { return _parent.get(); }

	// Signals
	sigc::signal<void, SharedPtr<ObjectModel> >    signal_new_child; 
	sigc::signal<void, SharedPtr<ObjectModel> >    signal_removed_child; 
	sigc::signal<void, const string&, const Atom&> signal_variable; 
	sigc::signal<void, const string&, const Atom&> signal_property; 
	sigc::signal<void>                             signal_destroyed; 
	sigc::signal<void>                             signal_renamed; 

protected:
	friend class ClientStore;
	
	ObjectModel(const Path& path);
	
	virtual void set_path(const Path& p) { _path = p; signal_renamed.emit(); }
	virtual void set_parent(SharedPtr<ObjectModel> p) { assert(p); _parent = p; }
	virtual void add_child(SharedPtr<ObjectModel> c) {}
	virtual bool remove_child(SharedPtr<ObjectModel> c) { return true; }

	virtual void set(SharedPtr<ObjectModel> model);

	Path                   _path;
	SharedPtr<ObjectModel> _parent;
	
	Variables  _variables;
	Properties _properties;
};


} // namespace Client
} // namespace Ingen

#endif // OBJECTMODEL_H
