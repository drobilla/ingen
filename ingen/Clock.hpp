/*
  This file is part of Ingen.
  Copyright 2016-2017 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_ENGINE_CLOCK_HPP
#define INGEN_ENGINE_CLOCK_HPP

#ifdef __MACH__
#    include <mach/mach.h>
#    include <mach/mach_time.h>
#else
#    include <sys/time.h>
#    include <time.h>
#endif

#include <cstdint>

namespace ingen {

class Clock {
public:
#ifdef __MACH__

	Clock() { mach_timebase_info(&_timebase); }

	inline uint64_t now_microseconds() const {
		const uint64_t now = mach_absolute_time();
		return now * _timebase.numer / _timebase.denom / 1e3;
	}

private:
	mach_timebase_info_data_t _timebase;

#else

	inline uint64_t now_microseconds() const {
		struct timespec time;
#    if defined(CLOCK_MONOTONIC_RAW)
		clock_gettime(CLOCK_MONOTONIC_RAW, &time);
#    else
		clock_gettime(CLOCK_MONOTONIC, &time);
#    endif
		return static_cast<uint64_t>(time.tv_sec) * 1e6 +
		       static_cast<uint64_t>(time.tv_nsec) / 1e3;
	}

#endif
};

} // namespace ingen

#endif // INGEN_ENGINE_CLOCK_HPP
