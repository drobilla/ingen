/* This file is part of Ingen.
 * Copyright (C) 2008-2009 David Robillard <http://drobilla.net>
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

#ifndef INGEN_SHARED_RUNTIME_PATHS_HPP
#define INGEN_SHARED_RUNTIME_PATHS_HPP

#ifndef _GNU_SOURCE
	#define _GNU_SOURCE
#endif

#include <string>

namespace Ingen {
namespace Shared {

void set_bundle_path(const char* path);
void set_bundle_path_from_code(void* function);

std::string bundle_file_path(const std::string& name);
std::string data_file_path(const std::string& name);
std::string module_path(const std::string& name, std::string dir="");

} // namespace Ingen
} // namespace Shared

#endif // INGEN_SHARED_RUNTIME_PATHS_HPP
