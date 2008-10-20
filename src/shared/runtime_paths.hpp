/* This file is part of Ingen.
 * Copyright (C) 2008 Dave Robillard <http://drobilla.net>
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

#ifndef INGEN_DATA_FILE_HPP
#define INGEN_DATA_FILE_HPP

#ifndef _GNU_SOURCE
	#define _GNU_SOURCE
#endif

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string>
#include "config.h"
#include "shared/runtime_paths.hpp"

namespace Ingen {
namespace Shared {

void set_bundle_path_from_code(void* function);

std::string data_file_path(const std::string& name);
std::string module_path(const std::string& name);

} // namespace Ingen
} // namespace Shared

#endif // INGEN_DATA_FILE_HPP
