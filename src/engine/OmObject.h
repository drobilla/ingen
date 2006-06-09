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
	: m_parent(parent), m_name(name)
	{
		assert(parent == NULL || m_name.length() > 0);
		assert(parent == NULL || m_name.find("/") == string::npos);
		//assert(((string)path()).find("//") == string::npos);
	}
	
	virtual ~OmObject() {}
	
	// Ghetto home-brew RTTI
	virtual Patch* as_patch() { return NULL; }
	virtual Node*  as_node()  { return NULL; }
	virtual Port*  as_port()  { return NULL; }
	
	OmObject* parent() const { return m_parent; }

	inline const string& name() const { return m_name; }
	
	virtual void set_path(const Path& new_path) {
		m_name = new_path.name();
		assert(m_name.find("/") == string::npos);
	}
	
	void          set_metadata(const string& key, const string& value) { m_metadata[key] = value; }
	const string& get_metadata(const string& key) {
		static const string empty_string = "";
		map<string, string>::iterator i = m_metadata.find(key);
		if (i != m_metadata.end())
			return (*i).second;
		else
			return empty_string;
	}

	const map<string, string>& metadata() const { return m_metadata; }

	inline const Path path() const {
		if (m_parent == NULL)
			return Path(string("/").append(m_name));
		else if (m_parent->path() == "/")
			return Path(string("/").append(m_name));
		else
			return Path(m_parent->path() +"/"+ m_name);
	}

	/** Patch and Node override this to recursively add their children. */
	virtual void add_to_store() = 0;
	
	/** Patch and Node override this to recursively remove their children. */
	virtual void remove_from_store() = 0;

protected:
	OmObject() {}
	
	OmObject* m_parent;
	string    m_name;

private:	
	// Prevent copies (undefined)
	OmObject(const OmObject&);
	OmObject& operator=(const OmObject& copy);

	map<string, string> m_metadata;
};


} // namespace Om

#endif // OMOBJECT_H
