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

#include "ingen/Resource.hpp"
#include "ingen/URIs.hpp"
#include "raul/Atom.hpp"

using namespace std;

namespace Ingen {

void
Resource::add_property(const Raul::URI&  uri,
                       const Raul::Atom& value,
                       Graph             ctx)
{
	// Ignore duplicate statements
	typedef Resource::Properties::const_iterator iterator;
	const std::pair<iterator, iterator> range = _properties.equal_range(uri);
	for (iterator i = range.first; i != range.second && i != _properties.end(); ++i) {
		if (i->second == value && i->second.context() == ctx) {
			return;
		}
	}

	const Raul::Atom& v = _properties.insert(make_pair(uri, Property(value, ctx)))->second;
	on_property(uri, v);
}

const Raul::Atom&
Resource::set_property(const Raul::URI&  uri,
                       const Raul::Atom& value,
                       Resource::Graph   ctx)
{
	// Erase existing property in this context
	for (Properties::iterator i = _properties.find(uri);
	     (i != _properties.end()) && (i->first == uri);) {
		Properties::iterator next = i;
		++next;
		if (i->second.context() == ctx) {
			_properties.erase(i);
		}
		i = next;
	}

	// Insert new property
	const Raul::Atom& v = _properties.insert(make_pair(uri, Property(value, ctx)))->second;
	on_property(uri, v);
	return v;
}

void
Resource::remove_property(const Raul::URI& uri, const Raul::Atom& value)
{
	if (value == _uris.wildcard) {
		_properties.erase(uri);
	} else {
		for (Properties::iterator i = _properties.find(uri);
		     i != _properties.end() && (i->first == uri);
		     ++i) {
			if (i->second == value) {
				_properties.erase(i);
				return;
			}
		}
	}
}

bool
Resource::has_property(const Raul::URI& uri, const Raul::Atom& value) const
{
	Properties::const_iterator i = _properties.find(uri);
	for (; (i != _properties.end()) && (i->first == uri); ++i) {
		if (i->second == value) {
			return true;
		}
	}
	return false;
}

const Raul::Atom&
Resource::set_property(const Raul::URI& uri, const Raul::Atom& value) const
{
	return const_cast<Resource*>(this)->set_property(uri, value);
}

const Raul::Atom&
Resource::get_property(const Raul::URI& uri) const
{
	static const Raul::Atom nil;
	Properties::const_iterator i = _properties.find(uri);
	return (i != _properties.end()) ? i->second : nil;
}

bool
Resource::type(const URIs&       uris,
               const Properties& properties,
               bool&             patch,
               bool&             block,
               bool&             port,
               bool&             is_output)
{
	typedef Resource::Properties::const_iterator iterator;
	const std::pair<iterator, iterator> types_range = properties.equal_range(uris.rdf_type);

	patch = block = port = is_output = false;
	for (iterator i = types_range.first; i != types_range.second; ++i) {
		const Raul::Atom& atom = i->second;
		if (atom.type() != uris.forge.URI && atom.type() != uris.forge.URID) {
			continue; // Non-URI type, ignore garbage data
		}

		if (atom == uris.ingen_Patch) {
			patch = true;
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

	if (patch && block && !port) { // => patch
		block = false;
		return true;
	} else if (port && (patch || block)) { // nonsense
		port = false;
		return false;
	} else if (patch || block || port) { // recognized type
		return true;
	} else { // unknown
		return false;
	}
}

void
Resource::set_properties(const Properties& p)
{
	/* Note a simple loop that calls set_property is inappropriate here since
	   it will not correctly set multiple properties in p (notably rdf:type)
	*/

	// Erase existing properties with matching keys
	for (Properties::const_iterator i = p.begin(); i != p.end(); ++i) {
		_properties.erase(i->first);
	}

	// Set new properties
	add_properties(p);
}

void
Resource::add_properties(const Properties& p)
{
	typedef Resource::Properties::const_iterator iterator;
	for (iterator i = p.begin(); i != p.end(); ++i)
		add_property(i->first, i->second, i->second.context());
}

void
Resource::remove_properties(const Properties& p)
{
	typedef Resource::Properties::const_iterator iterator;
	for (iterator i = p.begin(); i != p.end(); ++i)
		remove_property(i->first, i->second);
}

Resource::Properties
Resource::properties(Resource::Graph ctx) const
{
	if (ctx == Resource::DEFAULT) {
		return properties();
	}

	typedef Resource::Properties::const_iterator iterator;

	Properties props;
	for (iterator i = _properties.begin(); i != _properties.end(); ++i) {
		if (i->second.context() == Resource::DEFAULT
		    || i->second.context() == ctx) {
			props.insert(make_pair(i->first, i->second));
		}
	}

	return props;
}

} // namespace Ingen
