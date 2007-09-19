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

#ifndef GRAPHOBJECT_H
#define GRAPHOBJECT_H

#include <string>
#include <map>
#include <cstddef>
#include <cassert>
#include <raul/Deletable.hpp>
#include <raul/Path.hpp>
#include <raul/Atom.hpp>
#include "types.hpp"

using std::string;
using Raul::Atom;
using Raul::Path;


namespace Raul { class Maid; }

namespace Ingen {

class Patch;

/** An object on the audio graph - Patch, Node, Port, etc.
 *
 * Each of these is a Raul::Deletable and so can be deleted in a realtime safe
 * way from anywhere, and they all have a map of metadata for clients to store
 * arbitrary values in (which the engine puts no significance to whatsoever).
 *
 * \ingroup engine
 */
class GraphObject : public Raul::Deletable
{
public:
	typedef std::map<string, Atom> MetadataMap;

	GraphObject(GraphObject* parent, const string& name, bool polyphonic=false)
		: _parent(parent), _name(name), _polyphonic(polyphonic)
	{
		assert(parent == NULL || _name.length() > 0);
		assert(_name.find("/") == string::npos);
		assert(path().find("//") == string::npos);
	}
	
	virtual ~GraphObject() {}
	
	bool         polyphonic() const                       { return _polyphonic; }
	virtual void set_polyphonic(Raul::Maid& maid, bool p) { _polyphonic = p; }
	
	inline GraphObject*  parent() const { return _parent; }
	inline const string& name()   const { return _name; }
	
	virtual void process(SampleCount nframes, FrameTime start, FrameTime end) = 0;

	/** Rename */
	virtual void set_path(const Path& new_path) {
		assert(new_path.parent() == path().parent());
		_name = new_path.name();
		assert(_name.find("/") == string::npos);
	}
	
	void set_metadata(const string& key, const Atom& value)
	{ _metadata[key] = value; }

	const Atom& get_metadata(const string& key) {
		static Atom null_atom;
		MetadataMap::iterator i = _metadata.find(key);
		return (i != _metadata.end()) ? (*i).second : null_atom;
	}

	const MetadataMap& metadata() const { return _metadata; }

	/** The Patch this object is a child of. */
	virtual Patch* parent_patch() const;
	
	/** Path is dynamically generated from parent to ease renaming */
	inline const Path path() const {
		if (_parent == NULL)
			return Path(string("/").append(_name));
		else if (_parent->path() == "/")
			return Path(string("/").append(_name));
		else
			return Path(_parent->path() +"/"+ _name);
	}

protected:
	GraphObject* _parent;
	string       _name;
	bool         _polyphonic;

private:	
	MetadataMap _metadata;
};


} // namespace Ingen

#endif // GRAPHOBJECT_H
