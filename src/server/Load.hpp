/*
  This file is part of Ingen.
  Copyright 2007-2017 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_ENGINE_LOAD_HPP
#define INGEN_ENGINE_LOAD_HPP

#include <cmath>
#include <cstdint>
#include <limits>

namespace ingen {
namespace server {

struct Load {
	void update(uint64_t time, uint64_t available) {
		const uint64_t load = time * 100 / available;
		if (load < min) {
			min     = load;
			changed = true;
		}
		if (load > max) {
			max     = load;
			changed = true;
		}
		if (++n == 1) {
			mean    = load;
			changed = true;
		} else {
			const float a = mean + (static_cast<float>(load) - mean) /
			                           static_cast<float>(++n);

			if (a != mean) {
				changed = floorf(a) != floorf(mean);
				mean    = a;
			}
		}
	}

	uint64_t min     = std::numeric_limits<uint64_t>::max();
	uint64_t max     = 0;
	float    mean    = 0.0f;
	uint64_t n       = 0;
	bool     changed = false;
};

} // namespace server
} // namespace ingen

#endif // INGEN_ENGINE_LOAD_HPP
