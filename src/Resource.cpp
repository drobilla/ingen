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

#include <utility>

#include "ingen/Atom.hpp"
#include "ingen/Resource.hpp"
#include "ingen/URIs.hpp"

using namespace std;

namespace Ingen {

void
Resource::add_property(const Raul::URI& uri,
                       const Atom&      value,
                       Graph            ctx)
{
	// Ignore duplicate statements
	typedef Resource::Properties::const_iterator iterator;
	const std::pair<iterator, iterator> range = _properties.equal_range(uri);
	for (iterator i = range.first; i != range.second && i != _properties.end(); ++i) {
		if (i->second == value && i->second.context() == ctx) {
			return;
		}
	}

	const Atom& v = _properties.insert(make_pair(uri, Property(value, ctx)))->second;
	on_property(uri, v);
}

const Atom&
Resource::set_property(const Raul::URI& uri,
                       const Atom&      value,
                       Resource::Graph  ctx)
{
	// Erase existing property in this context
	for (Properties::iterator i = _properties.find(uri);
	     (i != _properties.end()) && (i->first == uri);) {
		Properties::iterator next = i;
		++next;
		if (i->second.context() == ctx) {
			const Atom value(i->second);
			_properties.erase(i);
			on_property_removed(uri, value);
		}
		i = next;
	}

	// Insert new property
	const Atom& v = _properties.insert(make_pair(uri, Property(value, ctx)))->second;
	on_property(uri, v);
	return v;
}

void
Resource::remove_property(const Raul::URI& uri, const Atom& value)
{
	if (value == _uris.patch_wildcard) {
		_properties.erase(uri);
	} else {
		for (Properties::iterator i = _properties.find(uri);
		     i != _properties.end() && (i->first == uri);
		     ++i) {
			if (i->second == value) {
				_properties.erase(i);
				break;
			}
		}
	}
	on_property_removed(uri, value);
}

bool
Resource::has_property(const Raul::URI& uri, const Atom& value) const
{
	Properties::const_iterator i = _properties.find(uri);
	for (; (i != _properties.end()) && (i->first == uri); ++i) {
		if (i->second == value) {
			return true;
		}
	}
	return false;
}

const Atom&
Resource::set_property(const Raul::URI& uri, const Atom& value) const
{
	return const_cast<Resource*>(this)->set_property(uri, value);
}

const Atom&
Resource::get_property(const Raul::URI& uri) const
{
	static const Atom nil;
	Properties::const_iterator i = _properties.find(uri);
	return (i != _properties.end()) ? i->second : nil;
}

bool
Resource::type(const URIs&       uris,
               const Properties& properties,
               bool&             graph,
               bool&             block,
               bool&             port,
               bool&             is_output)
{
	typedef Resource::Properties::const_iterator iterator;
	const std::pair<iterator, iterator> types_range = properties.equal_range(uris.rdf_type);

	graph = block = port = is_output = false;
	for (iterator i = types_range.first; i != types_range.second; ++i) {
		const Atom& atom = i->second;
		if (atom.type() != uris.forge.URI && atom.type() != uris.forge.URID) {
			continue; // Non-URI type, ignore garbage data
		}

		if (atom == uris.ingen_Graph) {
			graph = true;
		} else if (atom == uris.ingen_Block) {
			block = true;
		} else if (atom == uris.lv2_InputPort) {
			port = true;
			is_output = false;
		} else if (atom == uris.lv2_OutputPort) {
			port = true;
			is_output = true;
		}
	}

	if (graph && block && !port) { // => graph
		block = false;
		return true;
	} else if (port && (graph || block)) { // nonsense
		port = false;
		return false;
	} else if (graph || block || port) { // recognized type
		return true;
	} else { // unknown
		return false;
	}
}

void
Resource::set_properties(const Properties& props)
{
	/* Note a simple loop that calls set_property is inappropriate here since
	   it will not correctly set multiple properties in p (notably rdf:type)
	*/

	// Erase existing properties with matching keys
	for (const auto& p : props) {
		_properties.erase(p.first);
		on_property_removed(p.first, _uris.patch_wildcard);
	}

	// Set new properties
	add_properties(props);
}

void
Resource::add_properties(const Properties& props)
{
	for (const auto& p : props)
		add_property(p.first, p.second, p.second.context());
}

void
Resource::remove_properties(const Properties& props)
{
	for (const auto& p : props)
		remove_property(p.first, p.second);
}

Resource::Properties
Resource::properties(Resource::Graph ctx) const
{
	if (ctx == Resource::Graph::DEFAULT) {
		return properties();
	}

	Properties props;
	for (const auto& p : _properties) {
		if (p.second.context() == Resource::Graph::DEFAULT
		    || p.second.context() == ctx) {
			props.insert(make_pair(p.first, p.second));
		}
	}

	return props;
}

} // namespace Ingen
