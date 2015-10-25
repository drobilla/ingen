/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_RESOURCE_HPP
#define INGEN_RESOURCE_HPP

#include <map>
#include <string>

#include "ingen/Atom.hpp"
#include "ingen/URIs.hpp"
#include "ingen/ingen.h"
#include "raul/Deletable.hpp"
#include "raul/URI.hpp"

namespace Ingen {

/** An object with a URI described by properties.
 * @ingroup Ingen
 */
class INGEN_API Resource : public Raul::Deletable
{
public:
	Resource(const URIs& uris, const Raul::URI& uri)
		: _uris(uris)
		, _uri(uri)
	{}

	Resource& operator=(const Resource& rhs) {
		assert(&rhs._uris == &_uris);
		if (&rhs != this) {
			_uri        = rhs._uri;
			_properties = rhs._properties;
		}
		return *this;
	}

	enum class Graph {
		DEFAULT,
		EXTERNAL,
		INTERNAL
	};

	static Raul::URI graph_to_uri(Graph g) {
		switch (g) {
		case Graph::DEFAULT:  return Raul::URI(INGEN_NS "defaultContext");
		case Graph::EXTERNAL: return Raul::URI(INGEN_NS "externalContext");
		case Graph::INTERNAL: return Raul::URI(INGEN_NS "internalContext");
		}
	}

	static Graph uri_to_graph(const char* uri) {
		const char* suffix = uri + sizeof(INGEN_NS) - 1;
		if (strncmp(uri, INGEN_NS, sizeof(INGEN_NS) - 1)) {
			return Graph::DEFAULT;
		} else if (!strcmp(suffix, "defaultContext")) {
			return Graph::DEFAULT;
		} else if (!strcmp(suffix, "externalContext")) {
			return Graph::EXTERNAL;
		} else if (!strcmp(suffix, "internalContext")) {
			return Graph::INTERNAL;
		} else {
			return Graph::DEFAULT;
		}
	}

	/** A property value (an Atom with a context). */
	class Property : public Atom {
	public:
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

	virtual ~Resource() {}

	class Properties : public std::multimap<Raul::URI, Property> {
	public:
		Properties() {}

		Properties(const Properties& copy)
			: std::multimap<Raul::URI, Property>(copy)
		{}

		Properties(std::initializer_list<value_type> l)
			: std::multimap<Raul::URI, Property>(l)
		{}

		void put(const Raul::URI& key,
		         const Atom&      value,
		         Graph            ctx = Graph::DEFAULT) {
			insert(std::make_pair(key, Property(value, ctx)));
		}

		void put(const Raul::URI&   key,
		         const URIs::Quark& value,
		         Graph              ctx = Graph::DEFAULT) {
			insert(std::make_pair(key, Property(value, ctx)));
		}
	};

	/** Get a single property value.
	 *
	 * This is only useful for properties with a single value.  If the
	 * requested property has several values, the first will be returned.
	 */
	virtual const Atom& get_property(const Raul::URI& uri) const;

	/** Set (replace) a property value.
	 *
	 * This will first erase any properties with the given `uri`, so after
	 * this call exactly one property with predicate `uri` will be set.
	 */
	virtual const Atom& set_property(
		const Raul::URI& uri,
		const Atom&      value,
		Graph            ctx = Graph::DEFAULT);

	/** Set (replace) a property value.
	 *
	 * This will first erase any properties with the given `uri`, so after
	 * this call exactly one property with predicate `uri` will be set.
	 */
	virtual const Atom& set_property(
		const Raul::URI&   uri,
		const URIs::Quark& value,
		Graph              ctx = Graph::DEFAULT);

	/** Add a property value.
	 *
	 * This will not remove any existing values, so if properties with
	 * predicate `uri` and values other than `value` exist, this will result
	 * in multiple values for the property.
	 */
	virtual void add_property(const Raul::URI& uri,
	                          const Atom&      value,
	                          Graph            ctx = Graph::DEFAULT);

	/** Remove a property.
	 *
	 *	If `value` is patch:wildcard then any property with `uri` for a
	 *  predicate will be removed.
	 */
	virtual void remove_property(const Raul::URI& uri,
	                             const Atom&      value);

	/** Remove a property.
	 *
	 *	If `value` is patch:wildcard then any property with `uri` for a
	 *  predicate will be removed.
	 */
	virtual void remove_property(const Raul::URI&   uri,
	                             const URIs::Quark& value);

	/** Return true iff a property is set with a specific value. */
	virtual bool has_property(const Raul::URI& uri,
	                          const Atom&      value) const;

	/** Return true iff a property is set with a specific value. */
	virtual bool has_property(const Raul::URI&   uri,
	                          const URIs::Quark& value) const;

	/** Set (replace) several properties at once.
	 *
	 * This will erase all properties with keys in `p`, though multiple values
	 * for one property may exist in `p` will all be set (unlike simply
	 * calling set_property in a loop which would only set one value).
	 */
	void set_properties(const Properties& p);

	/** Add several properties at once. */
	void add_properties(const Properties& p);

	/** Remove several properties at once.
	 *
	 * This removes all matching properties (both key and value), or all
	 * properties with a matching key if the value in `p` is patch:wildcard.
	 */
	void remove_properties(const Properties& p);

	/** Hook called whenever a property is added.
	 *
	 * This can be used by derived classes to implement special behaviour for
	 * particular properties (e.g. ingen:value for ports).
	 */
	virtual void on_property(const Raul::URI& uri, const Atom& value) {}

	/** Hook called whenever a property value is removed.
	 *
	 * If all values for a key are removed, then value will be the wildcard.
	 *
	 * This can be used by derived classes to implement special behaviour for
	 * particular properties (e.g. ingen:value for ports).
	 */
	virtual void on_property_removed(const Raul::URI& uri, const Atom& value) {}

	/** Get the ingen type from a set of Properties.
	 *
	 * If some coherent ingen type is found, true is returned and the appropriate
	 * output parameter set to true.  Otherwise false is returned.
	 */
	static bool type(const URIs&       uris,
	                 const Properties& properties,
	                 bool&             graph,
	                 bool&             block,
	                 bool&             port,
	                 bool&             is_output);

	virtual void set_uri(const Raul::URI& uri) { _uri = uri; }

	/** Get all the properties with a given context. */
	Properties properties(Resource::Graph ctx) const;

	const URIs&       uris()       const { return _uris; }
	const Raul::URI&  uri()        const { return _uri; }
	const Properties& properties() const { return _properties; }
	Properties&       properties()       { return _properties; }

protected:
	const Atom& set_property(const Raul::URI& uri, const Atom& value) const;

	const URIs& _uris;

private:
	Raul::URI          _uri;
	mutable Properties _properties;
};

} // namespace Ingen

#endif // INGEN_RESOURCE_HPP
