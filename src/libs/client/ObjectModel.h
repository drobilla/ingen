/* This file is part of Om.  Copyright (C) 2005 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

using std::string; using std::map; using std::find;
using std::cout; using std::cerr; using std::endl;
using Om::Path;

namespace LibOmClient {

class ObjectController;

	
/** Base class for all OmObject models (NodeModel, PatchModel, PortModel).
 *
 * \ingroup libomclient
 */
class ObjectModel
{
public:
	ObjectModel(const string& path);
	ObjectModel() : m_path("/UNINITIALIZED"), m_parent(NULL) {} // FIXME: remove

	virtual ~ObjectModel() {}
	
	const map<string, string>& metadata() const { return m_metadata; }
	string get_metadata(const string& key) const;
	void   set_metadata(const string& key, const string& value)
		{ assert(value.length() > 0); m_metadata[key] = value; metadata_update_sig.emit(key, value); }
	
	inline const Path& path() const            { return m_path; }
	virtual void       set_path(const Path& p) { m_path = p; }
	
	ObjectModel*  parent() const             { return m_parent; }
	virtual void  set_parent(ObjectModel* p) { m_parent = p; }
	
	ObjectController* controller() const { return m_controller; }
	
	void set_controller(ObjectController* c);

	// Convenience functions
	string        base_path() const;
	const string  name() const { return m_path.name(); }
	
	// Signals
	sigc::signal<void, const string&, const string&> metadata_update_sig; 
protected:
	Path              m_path;
	ObjectModel*      m_parent;
	ObjectController* m_controller;
	
	map<string,string> m_metadata;

private:
	// Prevent copies (undefined)
	ObjectModel(const ObjectModel& copy);
	ObjectModel& operator=(const ObjectModel& copy);
};


} // namespace LibOmClient

#endif // OBJECTMODEL_H
