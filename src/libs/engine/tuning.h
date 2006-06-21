/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef TUNING_H
#define TUNING_H

#include <stddef.h>
#include <time.h>

namespace Om {
	
// FIXME: put this in a Config class
	
static const size_t event_queue_size           = 1024;
static const size_t pre_processor_queue_size   = 1024;
static const size_t post_processor_queue_size  = 1024;
static const size_t maid_queue_size            = 1024;

// This controls both the LASH event processing rate and the Maid cleanup rate
// (both of which are driven from the main thread)
static const timespec main_rate       = { 0, 500000000 }; // 1/2 second


} // namespace Om

#endif // TUNING_H
