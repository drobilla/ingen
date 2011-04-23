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

#ifndef INGEN_INTERFACE_RESOURCE_HPP
#define INGEN_INTERFACE_RESOURCE_HPP

#include <map>
#include <string>

#include "raul/Atom.hpp"
#include "raul/URI.hpp"

namespace Ingen {

class Resource
{
public:
	enum Graph {
		DEFAULT,
		EXTERNAL,
		INTERNAL
	};

	class Property : public Raul::Atom {
	public:
		Property(const Raul::Atom& atom, Graph ctx=DEFAULT)
			: Raul::Atom(atom)
			, _ctx(ctx)
		{}

		Property()                       : Raul::Atom(),     _ctx(DEFAULT) {}
		Property(int32_t val)            : Raul::Atom(val),  _ctx(DEFAULT) {}
		Property(float val)              : Raul::Atom(val),  _ctx(DEFAULT) {}
		Property(bool val)               : Raul::Atom(val),  _ctx(DEFAULT) {}
		Property(const char* val)        : Raul::Atom(val),  _ctx(DEFAULT) {}
		Property(const std::string& val) : Raul::Atom(val),  _ctx(DEFAULT) {}

		Property(const Raul::Atom::DictValue& dict)
			: Raul::Atom(dict)
			, _ctx(DEFAULT)
		{}

		Property(Type t, const std::string& uri)
			: Raul::Atom(t, uri)
			, _ctx(DEFAULT)
		{}

		Graph context() const          { return _ctx; }
		void  set_context(Graph ctx) { _ctx = ctx; }

	private:
		Graph _ctx;
	};

	virtual ~Resource() {}

	virtual const Raul::URI& uri() const = 0;

	typedef std::multimap<Raul::URI, Property> Properties;

	static void set_context(Properties& props, Graph ctx) {
		for (Properties::iterator i = props.begin(); i != props.end(); ++i) {
			i->second.set_context(ctx);
		}
	}

	virtual Properties properties(Resource::Graph ctx) const = 0;

	virtual const Properties& properties() const = 0;
	virtual Properties&       properties()       = 0;
	virtual const Raul::Atom& get_property(const Raul::URI&  uri) const   = 0;
	virtual Raul::Atom&       set_property(const Raul::URI&  uri,
	                                       const Raul::Atom& value)       = 0;
	virtual void              add_property(const Raul::URI&  uri,
	                                       const Raul::Atom& value)       = 0;
	virtual bool              has_property(const Raul::URI&  uri,
	                                       const Raul::Atom& value) const = 0;
};

} // namespace Ingen

#endif // INGEN_INTERFACE_RESOURCE_HPP
