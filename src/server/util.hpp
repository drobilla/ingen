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

#include "ingen_config.h"

#include "ingen/Log.hpp"
#include "raul/Path.hpp"

#ifdef __SSE__
#include <xmmintrin.h>
#endif

#include <cstdlib>
#include <fenv.h>

#ifdef __clang__
#    define REALTIME __attribute__((annotate("realtime")))
#else
#    define REALTIME
#endif

#if defined(INGEN_HAVE_THREAD_LOCAL)
#    define INGEN_THREAD_LOCAL thread_local
#elif defined(INGEN_HAVE_THREAD_BUILTIN)
#    define INGEN_THREAD_LOCAL __thread
#else
#    define INGEN_THREAD_LOCAL
#endif

namespace ingen {
namespace server {

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

} // namespace server
} // namespace ingen

#endif // INGEN_ENGINE_UTIL_HPP
