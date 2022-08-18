/*
  This file is part of Ingen.
  Copyright 2007-2016 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ingen/Resource.hpp"

#include "ingen/Atom.hpp"
#include "ingen/Forge.hpp"
#include "ingen/URIs.hpp"

#include <map>
#include <utility>

namespace ingen {

bool
Resource::add_property(const URI& uri, const Atom& value, Graph ctx)
{
	// Ignore duplicate statements
	const auto range = _properties.equal_range(uri);
	for (auto i = range.first; i != range.second && i != _properties.end(); ++i) {
		if (i->second == value && i->second.context() == ctx) {
			return false;
		}
	}

	if (uri != _uris.ingen_activity) {
		// Insert new property
		const Atom& v = _properties.emplace(uri, Property(value, ctx))->second;
		on_property(uri, v);
	} else {
		// Announce ephemeral activity, but do not store
		on_property(uri, value);
	}

	return true;
}

const Atom&
Resource::set_property(const URI& uri, const Atom& value, Resource::Graph ctx)
{
	// Erase existing property in this context
	for (auto i = _properties.find(uri);
	     (i != _properties.end()) && (i->first == uri);) {
		auto next = i;
		++next;
		if (i->second.context() == ctx) {
			const auto old_value = i->second;
			_properties.erase(i);
			on_property_removed(uri, old_value);
		}
		i = next;
	}

	if (uri != _uris.ingen_activity) {
		// Insert new property
		const Atom& v = _properties.emplace(uri, Property(value, ctx))->second;
		on_property(uri, v);
		return v;
	}

	// Announce ephemeral activity, but do not store
	on_property(uri, value);
	return value;
}

const Atom&
Resource::set_property(const URI&         uri,
                       const URIs::Quark& value,
                       Resource::Graph    ctx)
{
	return set_property(uri, value.urid_atom(), ctx);
}

void
Resource::remove_property(const URI& uri, const Atom& value)
{
	if (_uris.patch_wildcard == value) {
		_properties.erase(uri);
	} else {
		for (auto i = _properties.find(uri);
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

void
Resource::remove_property(const URI& uri, const URIs::Quark& value)
{
	remove_property(uri, value.urid_atom());
	remove_property(uri, value.uri_atom());
}

bool
Resource::has_property(const URI& uri, const Atom& value) const
{
	return _properties.contains(uri, value);
}

bool
Resource::has_property(const URI& uri, const URIs::Quark& value) const
{
	for (auto i = _properties.find(uri);
	     (i != _properties.end()) && (i->first == uri);
	     ++i) {
		if (value == i->second) {
			return true;
		}
	}
	return false;
}

const Atom&
Resource::set_property(const URI& uri, const Atom& value) const
{
	return const_cast<Resource*>(this)->set_property(uri, value);
}

const Atom&
Resource::get_property(const URI& uri) const
{
	static const Atom nil;

	const auto i = _properties.find(uri);
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
	const auto types_range = properties.equal_range(uris.rdf_type);

	graph = block = port = is_output = false;
	for (auto i = types_range.first; i != types_range.second; ++i) {
		const Atom& atom = i->second;
		if (atom.type() != uris.forge.URI && atom.type() != uris.forge.URID) {
			continue; // Non-URI type, ignore garbage data
		}

		if (uris.ingen_Graph == atom) {
			graph = true;
		} else if (uris.ingen_Block == atom) {
			block = true;
		} else if (uris.lv2_InputPort == atom) {
			port = true;
			is_output = false;
		} else if (uris.lv2_OutputPort == atom) {
			port = true;
			is_output = true;
		}
	}

	if (graph && block && !port) { // => graph
		block = false;
		return true;
	}

	if (port && (graph || block)) { // nonsense
		port = false;
		return false;
	}

	return graph || block || port; // recognized type
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
		on_property_removed(p.first, _uris.patch_wildcard.urid_atom());
	}

	// Set new properties
	add_properties(props);
}

void
Resource::add_properties(const Properties& props)
{
	for (const auto& p : props) {
		add_property(p.first, p.second, p.second.context());
	}
}

void
Resource::remove_properties(const Properties& props)
{
	for (const auto& p : props) {
		remove_property(p.first, p.second);
	}
}

Properties
Resource::properties(Resource::Graph ctx) const
{
	if (ctx == Resource::Graph::DEFAULT) {
		return properties();
	}

	Properties props;
	for (const auto& p : _properties) {
		if (p.second.context() == ctx) {
			props.emplace(p.first, p.second);
		}
	}

	return props;
}

} // namespace ingen
