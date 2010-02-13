/* This file is part of Ingen.
 * Copyright (C) 2008-2009 Dave Robillard <http://drobilla.net>
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
#include <string>
#include <map>
#include "interface/CommonInterface.hpp"

namespace Raul { class Atom; class Path; }

namespace Ingen {
namespace Shared {

class Store;


/** A wrapper for a CommonInterface that creates objects but possibly maps
 * symbol names to avoid clashes with the existing objects in a store.
 */
class ClashAvoider : public CommonInterface
{
public:
	ClashAvoider(Store& store, CommonInterface& target, Store* also_avoid=NULL)
		: _store(store), _target(target), _also_avoid(also_avoid) {}

	void set_target(CommonInterface& target) { _target = target; }

	// Bundles
	void bundle_begin() { _target.bundle_begin(); }
	void bundle_end()   { _target.bundle_end(); }

	// Object commands

	virtual void put(const Raul::URI&            path,
	                 const Resource::Properties& properties);

	virtual void delta(const Raul::URI&                    path,
	                   const Shared::Resource::Properties& remove,
	                   const Shared::Resource::Properties& add);

	virtual void move(const Raul::Path& old_path,
	                  const Raul::Path& new_path);

	virtual void connect(const Raul::Path& src_port_path,
	                     const Raul::Path& dst_port_path);

	virtual void disconnect(const Raul::Path& src_port_path,
	                        const Raul::Path& dst_port_path);

	virtual void set_property(const Raul::URI&  subject_path,
	                          const Raul::URI&  predicate,
	                          const Raul::Atom& value);

	virtual void set_voice_value(const Raul::Path& port_path,
	                             uint32_t          voice,
	                             const Raul::Atom& value);

	virtual void del(const Raul::Path& path);

private:
	const Raul::URI  map_uri(const Raul::URI& in);
	const Raul::Path map_path(const Raul::Path& in);

	Store&            _store;
	CommonInterface&  _target;

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

