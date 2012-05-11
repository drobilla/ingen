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

#include "raul/Atom.hpp"
#include "raul/URI.hpp"
#include "raul/log.hpp"

#define NS_INGEN "http://drobilla.net/ns/ingen#"

namespace Ingen {

class Resource
{
public:
	enum Graph {
		DEFAULT,
		EXTERNAL,
		INTERNAL
	};

	static Raul::URI graph_to_uri(Graph g) {
		switch (g) {
		default:
		case DEFAULT:
			return NS_INGEN "defaultContext";
		case EXTERNAL:
			return NS_INGEN "externalContext";
		case INTERNAL:
			return NS_INGEN "internalContext";
		}
	}

	static Graph uri_to_graph(const char* uri) {
		if (strncmp(uri, NS_INGEN, sizeof(NS_INGEN) - 1)) {
			return DEFAULT;
		}

		const char* suffix = uri + sizeof(NS_INGEN) - 1;
		if (!strcmp(suffix, "defaultContext")) {
			return DEFAULT;
		} else if (!strcmp(suffix, "externalContext")) {
			return EXTERNAL;
		} else if (!strcmp(suffix, "internalContext")) {
			return INTERNAL;
		}

		Raul::error << "Unknown context URI " << uri << std::endl;
		return DEFAULT;
	}

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
	virtual const Raul::Atom& set_property(const Raul::URI&  uri,
	                                       const Raul::Atom& value,
	                                       Graph             ctx=DEFAULT) = 0;
	virtual void              add_property(const Raul::URI&  uri,
	                                       const Raul::Atom& value,
	                                       Graph             ctx=DEFAULT) = 0;
	virtual bool              has_property(const Raul::URI&  uri,
	                                       const Raul::Atom& value) const = 0;
};

} // namespace Ingen

#endif // INGEN_RESOURCE_HPP

