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

#ifndef GRAPHOBJECTIMPL_H
#define GRAPHOBJECTIMPL_H

#include <string>
#include <map>
#include <cstddef>
#include <cassert>
#include "raul/Deletable.hpp"
#include "raul/Path.hpp"
#include "interface/GraphObject.hpp"
#include "shared/ResourceImpl.hpp"

namespace Raul { class Maid; }

namespace Ingen {

class PatchImpl;
class ProcessContext;


/** An object on the audio graph - Patch, Node, Port, etc.
 *
 * Each of these is a Raul::Deletable and so can be deleted in a realtime safe
 * way from anywhere, and they all have a map of variable for clients to store
 * arbitrary values in (which the engine puts no significance to whatsoever).
 *
 * \ingroup engine
 */
class GraphObjectImpl : virtual public Ingen::Shared::GraphObject
                      , public Ingen::Shared::ResourceImpl
{
public:
	virtual ~GraphObjectImpl() {}
	
	bool         polyphonic() const                       { return _polyphonic; }
	virtual bool set_polyphonic(Raul::Maid& maid, bool p) { _polyphonic = p; return true; }
	
	GraphObject* graph_parent() const { return _parent; }
	
	const Raul::URI uri() const { return path(); }

	inline GraphObjectImpl* parent() const { return _parent; }
	const Raul::Symbol      symbol() const { return _name; }
	
	virtual void process(ProcessContext& context) = 0;

	/** Rename */
	virtual void set_path(const Raul::Path& new_path) {
		assert(new_path.parent() == path().parent());
		_name = new_path.name();
		assert(_name.find("/") == std::string::npos);
	}
	
	const Raul::Atom& get_variable(const Raul::URI& key);
	void              set_variable(const Raul::URI& key, const Raul::Atom& value);
	
	const Properties&  variables()  const { return _variables; }
	Properties&        variables()        { return _variables; }

	/** The Patch this object is a child of. */
	virtual PatchImpl* parent_patch() const;
	
	/** Raul::Path is dynamically generated from parent to ease renaming */
	const Raul::Path path() const {
		if (_parent == NULL)
			return Raul::Path(std::string("/").append(_name));
		else if (_parent->path().is_root())
			return Raul::Path(std::string("/").append(_name));
		else
			return Raul::Path(_parent->path().child(_name));
	}
	
	SharedPtr<GraphObject> find_child(const std::string& name) const;

protected:
	GraphObjectImpl(GraphObjectImpl* parent, const std::string& name, bool polyphonic=false)
		: ResourceImpl((parent ? parent->path().base() : Raul::Path::root_uri) + name)
		, _parent(parent)
		, _name(name)
		, _polyphonic(polyphonic)
	{
		assert(parent == NULL || _name.length() > 0);
		assert(_name.find("/") == std::string::npos);
	}
	
	GraphObjectImpl* _parent;
	std::string      _name;
	bool             _polyphonic;

private:
	Properties _variables;
};


} // namespace Ingen

#endif // GRAPHOBJECTIMPL_H
