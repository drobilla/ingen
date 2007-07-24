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

#ifndef INGEN_CLIENT_H
#define INGEN_CLIENT_H

#include <raul/SharedPtr.hpp>

namespace Ingen {

class Engine;

namespace Shared { class EngineInterface; }

namespace Client {

extern "C" {

	SharedPtr<Shared::EngineInterface> new_osc_interface(const std::string& url);
	SharedPtr<Shared::EngineInterface> new_queued_interface(SharedPtr<Ingen::Engine> engine);

}


} // namespace Client
} // namespace Ingen

#endif // INGEN_CLIENT_H

