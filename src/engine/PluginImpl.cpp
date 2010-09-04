/* This file is part of Ingen.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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
#include "PluginImpl.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {


void
PluginImpl::load()
{
	if (!_module) {
		debug << "Loading plugin library " << _library_path << endl;
		_module = new Glib::Module(_library_path, Glib::MODULE_BIND_LOCAL);
		if (!(*_module))
			delete _module;
	}
}


void
PluginImpl::unload()
{
	if (_module) {
		debug << "Unloading plugin library " << _library_path << endl;
		delete _module;
		_module = NULL;
	}
}


} // namespace Ingen

