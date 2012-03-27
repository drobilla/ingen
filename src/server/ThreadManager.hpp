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


#ifndef INGEN_ENGINE_THREADMANAGER_HPP
#define INGEN_ENGINE_THREADMANAGER_HPP

#include <cassert>
#include "raul/Thread.hpp"

namespace Ingen {
namespace Server {

enum ThreadID {
	THREAD_PRE_PROCESS,
	THREAD_PROCESS,
	THREAD_MESSAGE,
};

class ThreadManager {
public:
	inline static bool thread_is(ThreadID id) {
		return Raul::Thread::get().is_context(id);
	}

	inline static void assert_thread(ThreadID id) {
		assert(single_threaded || Raul::Thread::get().is_context(id));
	}

	inline static void assert_not_thread(ThreadID id) {
		assert(single_threaded || !Raul::Thread::get().is_context(id));
	}

	/** Set to true during initialisation so ensure_thread doesn't fail.
	 * Defined in Engine.cpp
	 */
	static bool single_threaded;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_THREADMANAGER_HPP
