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

#ifndef CONTROLMODEL_H
#define CONTROLMODEL_H

#include <string>
#include <raul/Path.h>

namespace Ingen {
namespace Client {

	
/** A single port's control setting (in a preset).
 *
 * \ingroup IngenClient
 */
class ControlModel
{
public:
	ControlModel(const Path& port_path, float value)
	: _port_path(port_path),
	  _value(value)
	{
		assert(_port_path.find("//") == string::npos);
	}
	
	const Path&   port_path() const          { return _port_path; }
	void          port_path(const string& p) { _port_path = p; }
	float         value() const              { return _value; }
	void          value(float v)             { _value = v; }

private:
	Path  _port_path;
	float _value;
};


} // namespace Client
} // namespace Ingen

#endif // CONTROLMODEL
