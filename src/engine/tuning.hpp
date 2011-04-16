/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
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

#ifndef INGEN_ENGINE_TUNING_HPP
#define INGEN_ENGINE_TUNING_HPP

#include <stddef.h>
#include <time.h>

namespace Ingen {

// FIXME: put this in a Config class

static const size_t event_queue_size           = 1024;
static const size_t pre_processor_queue_size   = 1024;
static const size_t post_processor_queue_size  = 1024;
static const size_t maid_queue_size            = 1024;
static const size_t message_context_queue_size = 1024;

static const size_t event_bytes_per_frame = 4;

} // namespace Ingen

#endif // INGEN_ENGINE_TUNING_HPP
