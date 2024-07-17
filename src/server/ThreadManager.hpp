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

#ifndef INGEN_ENGINE_THREADMANAGER_HPP
#define INGEN_ENGINE_THREADMANAGER_HPP

#include "server.h"

#include <cassert>

namespace ingen::server {

enum ThreadFlag {
	THREAD_IS_REAL_TIME = 1,
	THREAD_PRE_PROCESS  = 1 << 1,
	THREAD_PROCESS      = 1 << 2,
	THREAD_MESSAGE      = 1 << 3,
};

class INGEN_SERVER_API ThreadManager
{
public:
	static void set_flag(ThreadFlag f) {
#ifndef NDEBUG
		flags |= static_cast<unsigned>(f);
#endif
	}

	static void unset_flag(ThreadFlag f) {
#ifndef NDEBUG
		flags &= ~static_cast<unsigned>(f);
#endif
	}

	static void assert_thread(ThreadFlag f) {
		assert(single_threaded || (flags & f));
	}

	static void assert_not_thread(ThreadFlag f) {
		assert(single_threaded || !(flags & f));
	}

	/** Set to true during initialisation so ensure_thread doesn't fail.
	 * Defined in Engine.cpp
	 */
	static bool                  single_threaded;
	static thread_local unsigned flags;
};

} // namespace ingen::server

#endif // INGEN_ENGINE_THREADMANAGER_HPP
