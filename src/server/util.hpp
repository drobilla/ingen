/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_ENGINE_UTIL_HPP
#define INGEN_ENGINE_UTIL_HPP

#include "ingen/Log.hpp"

#ifdef __SSE__
#include <xmmintrin.h> // IWYU pragma: keep
#endif

#ifdef __clang__
#    define REALTIME __attribute__((annotate("realtime")))
#else
#    define REALTIME
#endif

namespace ingen::server {

/** Set flags to disable denormal processing.
 */
inline void
set_denormal_flags(ingen::Log& log)
{
#ifdef __SSE__
	_mm_setcsr(_mm_getcsr() | 0x8040);
	log.info("Set SSE denormal-are-zero and flush-to-zero flags\n");
#endif
}

} // namespace ingen::server

#endif // INGEN_ENGINE_UTIL_HPP
