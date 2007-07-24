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
 * You should have received a copy of the GNU General Public License alongCont
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef PRESETMODEL_H
#define PRESETMODEL_H

#include <string>
#include <list>
#include "ControlModel.hpp"

using std::string; using std::list;

namespace Ingen {
namespace Client {


/** Model of a preset (a collection of control settings).
 *
 * \ingroup IngenClient
 */
class PresetModel
{
public:
	PresetModel(const string& base_path)
	: _base_path(base_path)
	{}

	/** Add a control value to this preset.  An empty string for a node_name
	 * means the port is on the patch itself (not a node in the patch).
	 */
	void add_control(const string& node_name,
	                 const string& port_name, float value)
	{
		if (node_name != "")
			_controls.push_back(ControlModel(_base_path + node_name +"/"+ port_name, value));
		else
			_controls.push_back(ControlModel(_base_path + port_name, value));
	}

	const string& name() const          { return _name; }
	void          name(const string& n) { _name = n; }

	const list<ControlModel>& controls() const { return _controls; }

private:
	string             _name;
	string             _base_path;
	list<ControlModel> _controls;
};


} // namespace Client
} // namespace Ingen

#endif // PRESETMODEL
