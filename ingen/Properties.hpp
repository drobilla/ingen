/*
  This file is part of Ingen.
  Copyright 2007-2017 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_PROPERTIES_HPP
#define INGEN_PROPERTIES_HPP

#include <initializer_list>
#include <map>
#include <utility>

#include "ingen/Atom.hpp"
#include "ingen/URIs.hpp"
#include "raul/URI.hpp"

namespace Ingen {

/** A property value (an Atom with a context). */
class Property : public Atom {
public:
	enum class Graph {
		DEFAULT,  ///< Default context for "universal" properties
		EXTERNAL, ///< Externally visible graph properties
		INTERNAL  ///< Internally visible graph properties
	};

	Property(const Atom& atom, Graph ctx=Graph::DEFAULT)
		: Atom(atom)
		, _ctx(ctx)
	{}

	Property(const URIs::Quark& quark, Graph ctx=Graph::DEFAULT)
		: Atom(quark.urid)
		, _ctx(ctx)
	{}

	Graph context() const        { return _ctx; }
	void  set_context(Graph ctx) { _ctx = ctx; }

private:
	Graph _ctx;
};

class Properties : public std::multimap<Raul::URI, Property> {
public:
	using Graph = Property::Graph;

	Properties() = default;
	Properties(const Properties& copy) = default;

	Properties(std::initializer_list<value_type> l)
		: std::multimap<Raul::URI, Property>(l)
	{}

	void put(const Raul::URI& key,
	         const Atom&      value,
	         Graph            ctx = Graph::DEFAULT) {
		emplace(key, Property(value, ctx));
	}

	void put(const Raul::URI&   key,
	         const URIs::Quark& value,
	         Graph              ctx = Graph::DEFAULT) {
		emplace(key, Property(value, ctx));
	}

	bool contains(const Raul::URI& key, const Atom& value) {
		for (const_iterator i = find(key); i != end() && i->first == key; ++i) {
			if (i->second == value) {
				return true;
			}
		}
		return false;
	}
};

} // namespace Ingen

#endif // INGEN_PROPERTIES_HPP
