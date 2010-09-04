/* This file is part of Ingen.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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

#ifndef INGEN_ENGINE_THREADMANAGER_HPP
#define INGEN_ENGINE_THREADMANAGER_HPP

#include <cassert>
#include "raul/Thread.hpp"

namespace Ingen {


enum ThreadID {
	THREAD_PRE_PROCESS,
	THREAD_PROCESS,
	THREAD_POST_PROCESS,
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


} // namespace Ingen

#endif // INGEN_ENGINE_THREADMANAGER_HPP
