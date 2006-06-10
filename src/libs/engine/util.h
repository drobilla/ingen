/* This file is part of Om.  Copyright (C) 2006 Dave Robillard.
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
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef UTIL_H
#define UTIL_H

#include <iostream>
#include <cstdlib>

#include <fenv.h>
#ifdef __SSE__
#include <xmmintrin.h>
#endif

using std::cerr; using std::endl;

namespace Om {

/** Set flags to disable denormal processing.
 */
inline void
set_denormal_flags()
{
#ifdef __SSE__
	unsigned long a, b, c, d;

	asm("cpuid": "=a" (a), "=b" (b), "=c" (c), "=d" (d) : "a" (1));
	if (d & 1<<25) { /* It has SSE support */
		_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);

		asm("cpuid": "=a" (a), "=b" (b), "=c" (c), "=d" (d) : "a" (0));
		if (b == 0x756e6547) { /* It's an Intel */
			int stepping, model, family, extfamily;

			family = (a >> 8) & 0xf;
			extfamily = (a >> 20) & 0xff;
			model = (a >> 4) & 0xf;
			stepping = a & 0xf;
			if (family == 15 && extfamily == 0 && model == 0 && stepping < 7) {
				return;
			}
		}
		asm("cpuid": "=a" (a), "=b" (b), "=c" (c), "=d" (d) : "a" (1));
		if (d & 1<<26) { /* bit 26, SSE2 support */
			_mm_setcsr(_mm_getcsr() | 0x40);
			//cerr << "Set SSE denormal fix flag." << endl;
		}
	} else {
		cerr << "This code has been built with SSE support, but your processor does"
			<< " not support the SSE instruction set." << endl << "Exiting." << endl;
		exit(EXIT_FAILURE);
	}
#endif
	// Set 387 control register via standard C99 interface
	fesetround(FE_TOWARDZERO);
}

} // namespace Om

#endif // UTIL_H
