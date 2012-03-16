/* This file is part of Ingen.
 * Copyright 2008-2011 David Robillard <http://drobilla.net>
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

#ifndef INGEN_SHARED_CLASHAVOIDER_HPP
#define INGEN_SHARED_CLASHAVOIDER_HPP

#include <inttypes.h>

#include <map>

#include "ingen/Interface.hpp"

namespace Raul { class Atom; class Path; }

namespace Ingen {
namespace Shared {

class Store;

/** A wrapper for an Interface that creates objects but possibly maps
 * symbol names to avoid clashes with the existing objects in a store.
 */
class ClashAvoider : public Interface
{
public:
	ClashAvoider(Store& store, Interface& target, Store* also_avoid=NULL)
		: _store(store), _target(target), _also_avoid(also_avoid) {}

	Raul::URI uri() const { return "ingen:ClientStore"; }

	void set_target(Interface& target) { _target = target; }

	// Bundles
	void bundle_begin() { _target.bundle_begin(); }
	void bundle_end()   { _target.bundle_end(); }

	// Object commands

	virtual void put(const Raul::URI&            path,
	                 const Resource::Properties& properties,
	                 Resource::Graph             ctx=Resource::DEFAULT);

	virtual void delta(const Raul::URI&            path,
	                   const Resource::Properties& remove,
	                   const Resource::Properties& add);

	virtual void move(const Raul::Path& old_path,
	                  const Raul::Path& new_path);

	virtual void connect(const Raul::Path& src_port_path,
	                     const Raul::Path& dst_port_path);

	virtual void disconnect(const Raul::URI& src,
	                        const Raul::URI& dst);

	virtual void disconnect_all(const Raul::Path& parent_patch_path,
	                            const Raul::Path& path);

	virtual void set_property(const Raul::URI&  subject_path,
	                          const Raul::URI&  predicate,
	                          const Raul::Atom& value);

	virtual void del(const Raul::URI& uri);

	virtual void set_response_id(int32_t id) {}
	virtual void ping() {}
	virtual void get(const Raul::URI& uri) {}
	virtual void response(int32_t id, Status status) {}
	virtual void error(const std::string& msg) {}

private:
	const Raul::URI  map_uri(const Raul::URI& in);
	const Raul::Path map_path(const Raul::Path& in);

	Store&     _store;
	Interface& _target;

	Store* _also_avoid;
	bool exists(const Raul::Path& path) const;

	typedef std::map<Raul::Path, unsigned> Offsets;
	Offsets _offsets;

	typedef std::map<Raul::Path, Raul::Path> SymbolMap;
	SymbolMap _symbol_map;
};

} // namespace Shared
} // namespace Ingen

#endif // INGEN_SHARED_CLASHAVOIDER_HPP

