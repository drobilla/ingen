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

#include "raul/log.hpp"
#include "raul/Atom.hpp"
#include "ingen/shared/LV2URIMap.hpp"
#include "ingen/shared/ResourceImpl.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Shared {

void
ResourceImpl::add_property(const Raul::URI&  uri,
                           const Raul::Atom& value,
                           Graph             ctx)
{
	// Ignore duplicate statements
	typedef Resource::Properties::const_iterator iterator;
	const std::pair<iterator,iterator> range = _properties.equal_range(uri);
	for (iterator i = range.first; i != range.second && i != _properties.end(); ++i)
		if (i->second == value && i->second.context() == ctx)
			return;

	_properties.insert(make_pair(uri, Property(value, ctx)));
}

const Raul::Atom&
ResourceImpl::set_property(const Raul::URI& uri, const Raul::Atom& value,
                           Resource::Graph ctx)
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
	return _properties.insert(make_pair(uri, Property(value, ctx)))->second;
}

void
ResourceImpl::remove_property(const Raul::URI& uri, const Raul::Atom& value)
{
	if (value == _uris.wildcard) {
		_properties.erase(uri);
	} else {
		Properties::iterator i = _properties.find(uri);
		for (; (i != _properties.end()) && (i->first == uri); ++i) {
			if (i->second == value) {
				_properties.erase(i);
				return;
			}
		}
	}
}

bool
ResourceImpl::has_property(const Raul::URI& uri, const Raul::Atom& value) const
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
ResourceImpl::set_property(const Raul::URI& uri, const Raul::Atom& value) const
{
	return const_cast<ResourceImpl*>(this)->set_property(uri, value);
}

const Raul::Atom&
ResourceImpl::get_property(const Raul::URI& uri) const
{
	static const Raul::Atom nil;
	Properties::const_iterator i = _properties.find(uri);
	return (i != _properties.end()) ? i->second : nil;
}

bool
ResourceImpl::type(const URIs&       uris,
                   const Properties& properties,
                   bool&             patch,
                   bool&             node,
                   bool&             port,
                   bool&             is_output)
{
	typedef Resource::Properties::const_iterator iterator;
	const std::pair<iterator,iterator> types_range = properties.equal_range(uris.rdf_type);

	patch = node = port = is_output = false;
	for (iterator i = types_range.first; i != types_range.second; ++i) {
		const Atom& atom = i->second;
		if (atom.type() != Atom::URI) {
			warn << "[ResourceImpl] Non-URI type " << atom << endl;
			continue;
		}

		if (atom == uris.ingen_Patch) {
			patch = true;
		} else if (atom == uris.ingen_Node) {
			node = true;
		} else if (atom == uris.lv2_InputPort) {
			port = true;
			is_output = false;
		} else if (atom == uris.lv2_OutputPort) {
			port = true;
			is_output = true;
		}
	}

	if (patch && node && !port) { // => patch
		node = false;
		return true;
	} else if (port && (patch || node)) { // nonsense
		port = false;
		return false;
	} else if (patch || node || port) { // recognized type
		return true;
	} else { // unknown
		return false;
	}
}

void
ResourceImpl::set_properties(const Properties& p)
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
ResourceImpl::add_properties(const Properties& p)
{
	typedef Resource::Properties::const_iterator iterator;
	for (iterator i = p.begin(); i != p.end(); ++i)
		add_property(i->first, i->second, i->second.context());
}

void
ResourceImpl::remove_properties(const Properties& p)
{
	typedef Resource::Properties::const_iterator iterator;
	for (iterator i = p.begin(); i != p.end(); ++i) {
		if (i->second == _uris.wildcard) {
			_properties.erase(i->first);
		} else {
			for (Properties::iterator j = _properties.find(i->first);
					(j != _properties.end()) && (j->first == i->first); ++j) {
				if (j->second == i->second) {
					_properties.erase(j);
					break;
				}
			}
		}
	}
}

void
ResourceImpl::dump(std::ostream& os) const
{
	typedef Resource::Properties::const_iterator iterator;
	os << _uri << " [" << endl;
	for (iterator i = _properties.begin(); i != _properties.end(); ++i) {
		os << "\t" << i->first << " " << i->second << " ;" << endl;
	}
	os << "]" << endl;
}

Resource::Properties
ResourceImpl::properties(Resource::Graph ctx) const
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

} // namespace Shared
} // namespace Ingen
