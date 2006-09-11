/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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
#include <map>
#include <iostream>
#include <string>
#include <algorithm>
#include <cassert>
#include <sigc++/sigc++.h>
#include "util/Path.h"
#include "util/CountedPtr.h"
#include "ObjectController.h"
using std::string; using std::map; using std::find;
using std::cout; using std::cerr; using std::endl;

namespace Ingen {
namespace Client {

class ObjectController;

	
/** Base class for all GraphObject models (NodeModel, PatchModel, PortModel).
 *
 * \ingroup IngenClient
 */
class ObjectModel
{
public:
	ObjectModel(const Path& path);
	ObjectModel() : m_path("/UNINITIALIZED") {} // FIXME: remove

	virtual ~ObjectModel() {}
	
	const map<string, string>& metadata() const { return m_metadata; }
	string get_metadata(const string& key) const;
	void   set_metadata(const string& key, const string& value)
		{ assert(value.length() > 0); m_metadata[key] = value; metadata_update_sig.emit(key, value); }
	
	inline const Path& path() const            { return m_path; }
	virtual void       set_path(const Path& p) { m_path = p; }
	
	CountedPtr<ObjectModel> parent() const             { return m_parent; }
	virtual void  set_parent(CountedPtr<ObjectModel> p) { m_parent = p; }
	
	virtual void add_child(CountedPtr<ObjectModel> c) = 0;

	CountedPtr<ObjectController> controller() const { return m_controller; }
	
	void set_controller(CountedPtr<ObjectController> c);

	void assimilate(CountedPtr<ObjectModel> model);

	// Signals
	sigc::signal<void, const string&, const string&> metadata_update_sig; 
	sigc::signal<void>                               destroyed_sig; 
protected:
	Path                         m_path;
	CountedPtr<ObjectModel>      m_parent;
	CountedPtr<ObjectController> m_controller;
	
	map<string,string> m_metadata;

private:
	// Prevent copies (undefined)
	ObjectModel(const ObjectModel& copy);
	ObjectModel& operator=(const ObjectModel& copy);
};


} // namespace Client
} // namespace Ingen

#endif // OBJECTMODEL_H
