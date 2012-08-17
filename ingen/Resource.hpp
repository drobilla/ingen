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

#ifndef INGEN_RESOURCE_HPP
#define INGEN_RESOURCE_HPP

#include <map>
#include <string>

#include "ingen/URIs.hpp"
#include "raul/Atom.hpp"
#include "raul/Deletable.hpp"
#include "raul/URI.hpp"

#define NS_INGEN "http://drobilla.net/ns/ingen#"

namespace Ingen {

/** An object with a URI described by properties.
 * @ingroup Ingen
 */
class Resource : public Raul::Deletable
{
public:
	Resource(URIs& uris, const Raul::URI& uri)
		: _uris(uris)
		, _uri(uri)
	{}

	enum Graph {
		DEFAULT,
		EXTERNAL,
		INTERNAL
	};

	static Raul::URI graph_to_uri(Graph g) {
		switch (g) {
		case DEFAULT:  return Raul::URI(NS_INGEN "defaultContext");
		case EXTERNAL: return Raul::URI(NS_INGEN "externalContext");
		case INTERNAL: return Raul::URI(NS_INGEN "internalContext");
		}
	}

	static Graph uri_to_graph(const char* uri) {
		const char* suffix = uri + sizeof(NS_INGEN) - 1;
		if (strncmp(uri, NS_INGEN, sizeof(NS_INGEN) - 1)) {
			return DEFAULT;
		} else if (!strcmp(suffix, "defaultContext")) {
			return DEFAULT;
		} else if (!strcmp(suffix, "externalContext")) {
			return EXTERNAL;
		} else if (!strcmp(suffix, "internalContext")) {
			return INTERNAL;
		} else {
			return DEFAULT;
		}
	}

	/** A property value (an Atom with a context). */
	class Property : public Raul::Atom {
	public:
		Property(const Raul::Atom& atom, Graph ctx=DEFAULT)
			: Raul::Atom(atom)
			, _ctx(ctx)
		{}

		Graph context() const        { return _ctx; }
		void  set_context(Graph ctx) { _ctx = ctx; }

	private:
		Graph _ctx;
	};

	virtual ~Resource() {}

	typedef std::multimap<Raul::URI, Property> Properties;

	/** Get a single property value.
	 *
	 * This is only useful for properties with a single value.  If the
	 * requested property has several values, the first will be returned.
	 */
	virtual const Raul::Atom& get_property(const Raul::URI& uri) const;

	/** Set (replace) a property value.
	 *
	 * This will first erase any properties with the given @p uri, so after
	 * this call exactly one property with predicate @p uri will be set.
	 */
	virtual const Raul::Atom& set_property(const Raul::URI&  uri,
	                                       const Raul::Atom& value,
	                                       Graph             ctx=DEFAULT);

	/** Add a property value.
	 *
	 * This will not remove any existing values, so if properties with
	 * predicate @p uri and values other than @p value exist, this will result
	 * in multiple values for the property.
	 */
	virtual void add_property(const Raul::URI&  uri,
	                          const Raul::Atom& value,
	                          Graph             ctx=DEFAULT);

	/** Remove a property.
	 *
	 *	If @p value is ingen:wildcard then any property with @p uri for a
	 *  predicate will be removed.
	 */
	virtual void remove_property(const Raul::URI&  uri,
	                             const Raul::Atom& value);

	/** Return true iff a property is set. */
	virtual bool has_property(const Raul::URI&  uri,
	                          const Raul::Atom& value) const;

	/** Set (replace) several properties at once.
	 *
	 * This will erase all properties with keys in @p p, though multiple values
	 * for one property may exist in @p and will all be set (unlike simply
	 * calling set_property in a loop which would only set one value).
	 */
	void set_properties(const Properties& p);

	/** Add several properties at once. */
	void add_properties(const Properties& p);

	/** Remove several properties at once.
	 *
	 * This removes all matching properties (both key and value), or all
	 * properties with a matching key if the value in @p is ingen:wildcard.
	 */
	void remove_properties(const Properties& p);

	/** Hook called whenever a property is added.
	 *
	 * This can be used by derived classes to implement special behaviour for
	 * particular properties (e.g. ingen:value for ports).
	 */
	virtual void on_property(const Raul::URI& uri, const Raul::Atom& value) {}

	/** Get the ingen type from a set of Properties.
	 *
	 * If some coherent ingen type is found, true is returned and the appropriate
	 * output parameter set to true.  Otherwise false is returned.
	 */
	static bool type(const URIs&       uris,
	                 const Properties& properties,
	                 bool&             patch,
	                 bool&             node,
	                 bool&             port,
	                 bool&             is_output);

	virtual void set_uri(const Raul::URI& uri) { _uri = uri; }

	/** Get all the properties with a given context. */
	Properties properties(Resource::Graph ctx) const;

	URIs&             uris()       const { return _uris; }
	const Raul::URI&  uri()        const { return _uri; }
	const Properties& properties() const { return _properties; }
	Properties&       properties()       { return _properties; }

protected:
	const Raul::Atom& set_property(const Raul::URI& uri, const Raul::Atom& value) const;

	URIs& _uris;

private:
	Raul::URI          _uri;
	mutable Properties _properties;
};

} // namespace Ingen

#endif // INGEN_RESOURCE_HPP
