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

#ifndef INGEN_SHARED_RESOURCEIMPL_HPP
#define INGEN_SHARED_RESOURCEIMPL_HPP

#include "ingen/Resource.hpp"
#include "ingen/shared/URIs.hpp"
#include "raul/SharedPtr.hpp"
#include "raul/URI.hpp"

namespace Ingen {
namespace Shared {

class URIs;

class ResourceImpl : virtual public Resource
{
public:
	ResourceImpl(URIs& uris, const Raul::URI& uri)
		: _uris(uris)
		, _uri(uri)
	{}

	URIs& uris() const { return _uris; }

	virtual void             set_uri(const Raul::URI& uri) { _uri = uri; }
	virtual const Raul::URI& uri()  const { return _uri; }

	const Properties& properties() const { return _properties; }
	Properties&       properties()       { return _properties; }

	Properties properties(Resource::Graph ctx) const;

	const Raul::Atom& get_property(const Raul::URI& uri) const;

	const Raul::Atom& set_property(const Raul::URI&  uri,
	                               const Raul::Atom& value,
	                               Resource::Graph   ctx=Resource::DEFAULT);

	/** Hook called whenever a property is added.
	 * This can be used by derived classes to implement special behaviour for
	 * particular properties (e.g. ingen:value for ports).
	 */
	virtual void on_property(const Raul::URI& uri, const Raul::Atom& value) {}

	void remove_property(const Raul::URI& uri, const Raul::Atom& value);
	bool has_property(const Raul::URI& uri, const Raul::Atom& value) const;
	void add_property(const Raul::URI&  uri,
	                  const Raul::Atom& value,
	                  Graph             ctx = DEFAULT);
	void set_properties(const Properties& p);
	void add_properties(const Properties& p);
	void remove_properties(const Properties& p);

	/** Get the ingen type from a set of Properties.
	 * If some coherent ingen type is found, true is returned and the appropriate
	 * output parameter set to true.  Otherwise false is returned.
	 */
	static bool type(const URIs&       uris,
	                 const Properties& properties,
	                 bool&             patch,
	                 bool&             node,
	                 bool&             port,
	                 bool&             is_output);

protected:
	const Raul::Atom& set_property(const Raul::URI& uri, const Raul::Atom& value) const;

	URIs& _uris;

private:
	Raul::URI          _uri;
	mutable Properties _properties;
};

} // namespace Shared
} // namespace Ingen

#endif // INGEN_SHARED_RESOURCEIMPL_HPP

