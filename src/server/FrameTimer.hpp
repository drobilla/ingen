/*
  This file is part of Ingen.
  Copyright 2017 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_ENGINE_FRAMETIMER_HPP
#define INGEN_ENGINE_FRAMETIMER_HPP

#include <cmath>
#include <cstdint>

namespace ingen::server {

/** Delay-locked loop for monotonic sample time.
 *
 * See "Using a DLL to filter time" by Fons Adriaensen
 * http://kokkinizita.linuxaudio.org/papers/usingdll.pdf
 */
class FrameTimer
{
public:
	static constexpr double PI        = 3.14159265358979323846;
	static constexpr double bandwidth = 1.0 / 8.0; // Hz
	static constexpr double us_per_s  = 1000000.0;

	FrameTimer(uint32_t period_size, uint32_t sample_rate)
	    : tper((static_cast<double>(period_size) /
	            static_cast<double>(sample_rate)) *
	           us_per_s)
	    , omega(2 * PI * bandwidth / us_per_s * tper)
	    , b(sqrt(2) * omega)
	    , c(omega * omega)
	    , nper(period_size)
	{}

	/** Update the timer for current real time `usec` and frame `frame`. */
	void update(uint64_t usec, uint64_t frame) {
		if (!initialized || frame != n1) {
			init(usec, frame);
			return;
		}

		// Calculate loop error
		const double e = (static_cast<double>(usec) - t1);

		// Update loop
		t0 = t1;
		t1 += b * e + e2;
		e2 += c * e;

		// Update frame counts
		n0 = n1;
		n1 += nper;
	}

	/** Return an estimate of the frame time for current real time `usec`. */
	uint64_t frame_time(uint64_t usec) const {
		if (!initialized) {
			return 0;
		}

		const double delta = static_cast<double>(usec) - t0;
		const double period = t1 - t0;
		return n0 + std::round(delta / period * nper);
	}

private:
	void init(uint64_t now, uint64_t frame) {
		// Init loop
		e2 = tper;
		t0 = now;
		t1 = t0 + e2;

		// Init sample counts
		n0 = frame;
		n1 = n0 + nper;

		initialized = true;
	}

	const double tper;
	const double omega;
	const double b;
	const double c;

	uint64_t nper        = 0u;
	double   e2          = 0.0;
	double   t0          = 0.0;
	double   t1          = 0.0;
	uint64_t n0          = 0u;
	uint64_t n1          = 0u;
	bool     initialized = false;
};

} // namespace ingen::server

#endif // INGEN_ENGINE_FRAMETIMER_HPP
