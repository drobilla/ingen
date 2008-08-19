/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#include <iostream>
#include <raul/TableImpl.hpp>
#include "interface/GraphObject.hpp"
#include "ObjectModel.hpp"

using namespace std;

namespace Ingen {
namespace Client {


ObjectModel::ObjectModel(const Path& path)
	: _path(path)
{
}


ObjectModel::~ObjectModel()
{
}
	

/** Get a variable for this object.
 *
 * @return Metadata value with key @a key, empty string otherwise.
 */
const Atom&
ObjectModel::get_variable(const string& key) const
{
	static const Atom null_atom;

	Variables::const_iterator i = _variables.find(key);
	if (i != _variables.end())
		return i->second;
	else
		return null_atom;
}


/** Get a variable for this object.
 *
 * @return Metadata value with key @a key, empty string otherwise.
 */
Atom&
ObjectModel::get_variable( string& key)
{
	static Atom null_atom;

	Variables::iterator i = _variables.find(key);
	if (i != _variables.end())
		return i->second;
	else
		return null_atom;
}


/** Get a property of this object.
 *
 * @return Metadata value with key @a key, empty string otherwise.
 */
const Atom&
ObjectModel::get_property(const string& key) const
{
	static const Atom null_atom;

	Properties::const_iterator i = _properties.find(key);
	if (i != _properties.end())
		return i->second;
	else
		return null_atom;
}


/** Get a property of this object.
 *
 * @return Metadata value with key @a key, empty string otherwise.
 */
Atom&
ObjectModel::get_property(const string& key)
{
	static Atom null_atom;

	Properties::iterator i = _properties.find(key);
	if (i != _properties.end())
		return i->second;
	else
		return null_atom;
}


bool
ObjectModel::polyphonic() const
{
	Properties::const_iterator i = _properties.find("ingen:polyphonic");
	return (i != _properties.end() && i->second.type() == Atom::BOOL && i->second.get_bool());
}


/** Merge the data of @a model with self, as much as possible.
 *
 * This will merge the two models, but with any conflict take the value in
 * @a model as correct.  The paths of the two models MUST be equal.
 */
void
ObjectModel::set(SharedPtr<ObjectModel> o)
{
	assert(_path == o->path());
	if (o->_parent)
		_parent = o->_parent;

	for (Variables::const_iterator v = o->variables().begin(); v != o->variables().end(); ++v) {
		Variables::const_iterator mine = _variables.find(v->first);
		if (mine != _variables.end())
			cerr << "WARNING:  " << _path << "Client/Server variable mismatch: " << v->first << endl;
		_variables[v->first] = v->second;
		signal_variable.emit(v->first, v->second);
	}
	
	for (Properties::const_iterator v = o->properties().begin(); v != o->properties().end(); ++v) {
		Properties::const_iterator mine = _properties.find(v->first);
		if (mine != _properties.end())
			cerr << "WARNING:  " << _path << "Client/Server property mismatch: " << v->first << endl;
		_properties[v->first] = v->second;
		signal_variable.emit(v->first, v->second);
	}
}


} // namespace Client
} // namespace Ingen

