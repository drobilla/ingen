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


#include "raul/log.hpp"
#include "PluginImpl.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Server {

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

} // namespace Server
} // namespace Ingen

