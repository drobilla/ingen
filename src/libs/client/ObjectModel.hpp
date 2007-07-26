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
#include <raul/Table.hpp>

using std::string; using std::find;
using Raul::Atom;
using Raul::Path;

namespace Ingen {
namespace Client {

typedef Raul::Table<string, Atom> MetadataMap;


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
class ObjectModel : boost::noncopyable
{
public:
	virtual ~ObjectModel();

	const Atom& get_metadata(const string& key) const;
	void set_metadata(const string& key, const Atom& value)
		{ _metadata.insert(make_pair(key, value)); metadata_update_sig.emit(key, value); }

	typedef Raul::Table<string, SharedPtr<ObjectModel> > Children;

	const MetadataMap&     metadata() const { return _metadata; }
	const Children&        children() const { return _children; }
	inline const Path&     path()     const { return _path; }
	SharedPtr<ObjectModel> parent()   const { return _parent; }

	SharedPtr<ObjectModel> get_child(const string& name) const;

	// Signals
	sigc::signal<void, const string&, const Atom&> metadata_update_sig; 
	sigc::signal<void, SharedPtr<ObjectModel> >    new_child_sig; 
	sigc::signal<void, SharedPtr<ObjectModel> >    removed_child_sig; 
	sigc::signal<void>                             destroyed_sig; 
	sigc::signal<void>                             renamed_sig; 

protected:
	friend class Store;
	
	ObjectModel(const Path& path);
	
	virtual void set_path(const Path& p) { _path = p; renamed_sig.emit(); }
	virtual void set_parent(SharedPtr<ObjectModel> p) { assert(p); _parent = p; }
	virtual void add_child(SharedPtr<ObjectModel> c);
	virtual bool remove_child(SharedPtr<ObjectModel> c);
	
	void add_metadata(const MetadataMap& data);
	
	void set(SharedPtr<ObjectModel> model);

	Path                   _path;
	SharedPtr<ObjectModel> _parent;
	
	MetadataMap _metadata;
	Children    _children;
};


} // namespace Client
} // namespace Ingen

#endif // OBJECTMODEL_H
