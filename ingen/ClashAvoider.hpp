/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_CLASHAVOIDER_HPP
#define INGEN_CLASHAVOIDER_HPP

#include <inttypes.h>

#include <map>
#include <string>

#include "ingen/Interface.hpp"

namespace Raul {
class Atom;
class Path;
}

namespace Ingen {

class Store;

/** A wrapper for an Interface that creates objects but possibly maps
 * symbol names to avoid clashes with the existing objects in a store.
 *
 * @ingroup IngenShared
 */
class ClashAvoider : public Interface
{
public:
	ClashAvoider(Store& store, Interface& target, Store* also_avoid=NULL)
		: _store(store), _target(target), _also_avoid(also_avoid) {}

	Raul::URI uri() const { return Raul::URI("ingen:ClientStore"); }

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

	virtual void connect(const Raul::Path& tail,
	                     const Raul::Path& head);

	virtual void disconnect(const Raul::Path& tail,
	                        const Raul::Path& head);

	virtual void disconnect_all(const Raul::Path& graph,
	                            const Raul::Path& path);

	virtual void set_property(const Raul::URI&  subject_path,
	                          const Raul::URI&  predicate,
	                          const Raul::Atom& value);

	virtual void del(const Raul::URI& uri);

	virtual void set_response_id(int32_t id) {}
	virtual void get(const Raul::URI& uri) {}
	virtual void response(int32_t id, Status status, const std::string& subject) {}
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

} // namespace Ingen

#endif // INGEN_CLASHAVOIDER_HPP

