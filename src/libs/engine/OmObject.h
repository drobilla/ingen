/* This file is part of Om.  Copyright (C) 2006 Dave Robillard.
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

#ifndef OMOBJECT_H
#define OMOBJECT_H

#include <string>
#include <map>
#include <cstddef>
#include <cassert>
#include "MaidObject.h"
#include "util/Path.h"

using std::string; using std::map;

namespace Om {

class Patch;
class Node;
class Port;


/** An object in the "synth space" of Om - Patch, Node, Port, etc.
 *
 * Each of these is a MaidObject and so can be deleted in a realtime safe
 * way from anywhere, and they all have a map of metadata for clients to store
 * arbitrary values in (which the engine puts no significance to whatsoever).
 *
 * \ingroup engine
 */
class OmObject : public MaidObject
{
public:
	OmObject(OmObject* parent, const string& name)
	: _parent(parent), _name(name)
	{
		assert(parent == NULL || _name.length() > 0);
		assert(parent == NULL || _name.find("/") == string::npos);
		//assert(((string)path()).find("//") == string::npos);
	}
	
	virtual ~OmObject() {}
	
	OmObject* parent() const { return _parent; }

	inline const string& name() const { return _name; }
	
	virtual void set_path(const Path& new_path) {
		_name = new_path.name();
		assert(_name.find("/") == string::npos);
	}
	
	void          set_metadata(const string& key, const string& value) { _metadata[key] = value; }
	const string& get_metadata(const string& key) {
		static const string empty_string = "";
		map<string, string>::iterator i = _metadata.find(key);
		if (i != _metadata.end())
			return (*i).second;
		else
			return empty_string;
	}

	const map<string, string>& metadata() const { return _metadata; }

	inline const Path path() const {
		if (_parent == NULL)
			return Path(string("/").append(_name));
		else if (_parent->path() == "/")
			return Path(string("/").append(_name));
		else
			return Path(_parent->path() +"/"+ _name);
	}

	/** Patch and Node override this to recursively add their children. */
	virtual void add_to_store() = 0;
	
	/** Patch and Node override this to recursively remove their children. */
	virtual void remove_from_store() = 0;

protected:
	OmObject() {}
	
	OmObject* _parent;
	string    _name;

private:	
	// Prevent copies (undefined)
	OmObject(const OmObject&);
	OmObject& operator=(const OmObject& copy);

	map<string, string> _metadata;
};


} // namespace Om

#endif // OMOBJECT_H
